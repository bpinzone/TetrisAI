import argparse
import cv2
import json
import numpy as np
from scipy import stats
import sys
import time
from pynput.keyboard import Listener, Key

interrupted = True
res = (640, 480)
fps = 60

def interupt(key):
    global interrupted
    if not interrupted:
        interrupted = True

def add_click(event, x, y, flags, param):
    if event == cv2.EVENT_LBUTTONDOWN:
        param.append((x, y))
    # if event == cv2.EVENT_RBUTTONDOWN:
    #     closest_ind = min(range(len(param)), key=lambda coord: abs(coord[0] - x) + abs(coord[1] - y))
    #     param[closest_ind] = (x, y)

def get_bounding_coords_from_corners(xy1, xy2):
    x1, y1 = xy1
    x2, y2 = xy2
    min_x, max_x = min(x1, x2), max(x1, x2)
    min_y, max_y = min(y1, y2), max(y1, y2)
    return ((min_x, min_y), (max_x, max_y))

def get_corner_clicks(image):
    print('Click hold corners, then board corners, then queue corners.')
    print('Do so in this order: upper left, upper right, lower right, lower left.')
    print('Then press a key to continue.')
    clicked_points = []
    window_name = 'Click the corners'
    cv2.imshow(window_name, image)
    cv2.setMouseCallback(window_name, add_click, clicked_points)
    cv2.waitKey(0)
    return clicked_points

def get_bw_mask(image, _): # bw_stats
    return image > 60
    # TODO did this work
    # means, stddevs = bw_stats
    # distances = np.abs((image[:, :, np.newaxis] - means[np.newaxis, np.newaxis, :]) / stddevs[np.newaxis, np.newaxis, :])
    # return distances[:, :, 1] < distances[:, :, 0]

def show_sections(image_mask, rows, cols):
    return np.vstack(
            [np.hstack(
                [cv2.copyMakeBorder(pic_cell.astype(np.uint8) * 255, 2, 2, 2, 2, cv2.BORDER_CONSTANT, None, 128) for pic_cell in np.array_split(pic_row, cols, axis=1)]
                ) for pic_row in np.array_split(image_mask, rows, axis=0)]
            )

default_video = 0 # '/dev/video0'

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-v', '--video', help='path to video file. Uses webcam otherwise', default=default_video)
    parser.add_argument('-t', '--make-template', help='Helps you create a template file. Use --video for the template video and --config for the clicked points in the template.', action='store_true')
    parser.add_argument('-c', '--config', help='path to the clicks configuration file. Creates a new_config otherwise.')
    args = parser.parse_args()

    # We will transform the images so they are of this size.
    # IF YOU CHANGE THESE YOU MUST REMAKE THE TEMPLATE FILE.
    im_dims = {
            # 'hold_pic': (125, 140),
            # 'board_pic': (500, 1000),
            # 'queue_pic': (125, 450)
            'hold_pic': (25, 28),
            'board_pic': (100, 200),
            'queue_pic': (25, 90)
            }
    hold_ideal_corners, board_ideal_corners, queue_ideal_corners = (np.array([
        [0, 0],
        [dim[0], 0],
        [dim[0], dim[1]],
        [0, dim[1]],
        ], dtype=np.float32) for dim in (im_dims['hold_pic'], im_dims['board_pic'], im_dims['queue_pic']))

    # Figure out where we are getting our images from:
    capture = cv2.VideoCapture(args.video)
    writer = None
    global res
    global fps
    if args.video == default_video:
        assert capture.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc('M','J','P','G'));
        assert capture.set(cv2.CAP_PROP_FRAME_WIDTH, res[0])
        assert capture.set(cv2.CAP_PROP_FRAME_HEIGHT, res[1])
        assert capture.set(cv2.CAP_PROP_FPS, int(fps))

    # Get the corner points of hold, board, and queue
    if args.config == None:
        ret, first_frame = capture.read()
        ret, first_frame = capture.read()
        ret, first_frame = capture.read()
        ret, first_frame = capture.read()
        ret, first_frame = capture.read()
        assert ret
        clicked_points = get_corner_clicks(first_frame)

        def get_means_stddevs(image):
            float_image = image.astype(np.float32)
            criteria = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 10, 1.0)
            grayscale_data = float_image.flatten()[:, np.newaxis]
            kmeans_ret, label, center = cv2.kmeans(grayscale_data, 2, None, criteria, 10, cv2.KMEANS_RANDOM_CENTERS)
            empty_pixel_group = np.argmin(center)
            full_pixel_group = np.argmax(center)
            image_means = center[[empty_pixel_group, full_pixel_group], 0]
            image_stddevs = np.array([
                    np.std(grayscale_data[label == empty_pixel_group]),
                    np.std(grayscale_data[label == full_pixel_group])
                    ])
            return image_means, image_stddevs

        board_observed_corners = np.array(clicked_points[4:8], dtype=np.float32)
        M_board = cv2.getPerspectiveTransform(board_observed_corners, board_ideal_corners)
        board_pic = cv2.warpPerspective(first_frame, M_board, im_dims['board_pic'])
        gray_board = cv2.cvtColor(board_pic, cv2.COLOR_BGR2GRAY)
        piece_stats = get_means_stddevs(gray_board)
        classified_board = get_bw_mask(gray_board, piece_stats)

        cv2.imshow('gray_board', gray_board)
        cv2.imshow('classified_board', classified_board.astype(np.uint8) * 255)

        save_file_name = 'last_click_coords.config' if not args.make_template else 'last_template_click_coords.config'
        assert(len(clicked_points) == 12)
        with open(save_file_name, 'w') as f:
            json.dump({ 'clicked_points': clicked_points, 'piece_means': piece_stats[0].tolist(), 'piece_stddevs': piece_stats[1].tolist()}, f)

        print(f'Configuration file written to {save_file_name}')
        return

    with open(args.config) as f:
        config_dict = json.load(f)
    clicked_points = config_dict['clicked_points']
    piece_stats = np.array(config_dict['piece_means']), np.array(config_dict['piece_stddevs'])
    assert(len(clicked_points) == 12)

    hold_observed_corners = np.array(clicked_points[:4], dtype=np.float32)
    board_observed_corners = np.array(clicked_points[4:8], dtype=np.float32)
    queue_observed_corners = np.array(clicked_points[8:], dtype=np.float32)

    M_hold = cv2.getPerspectiveTransform(hold_observed_corners, hold_ideal_corners)
    M_board = cv2.getPerspectiveTransform(board_observed_corners, board_ideal_corners)
    M_queue = cv2.getPerspectiveTransform(queue_observed_corners, queue_ideal_corners)

    # If template argument is chosen, we make a template config file.
    if args.make_template:
        print('Time to make a template.')
        print('First, type the video file that you would like to use to generate the template config.')
        assert args.video # this should be a recorded video, not a webcam stream
        template_cap = cv2.VideoCapture(args.video)

        ret, frame = template_cap.read()

        params = set()
        requirements = ['red_l', 'orange_l', 'yellow_l', 'green_l', 'cyan_l','blue_l', 'purple_l',
                        'red_b', 'orange_b', 'yellow_b', 'green_b', 'cyan_b','blue_b', 'purple_b']
        while ret:
            cv2.imshow('frame', frame)
            cv2.waitKey(0)
            print(f'Requirements remaining: {[r for r in requirements if r not in params]}')
            print('Type "help" for help.')
            command = input()
            
            if command == 'q':
                break
            elif command[0:4] == 'set ':
                param = command[4:]
                if not param in requirements:
                    print(f'{param} is not a valid parameter.')
                    continue
                hold_pic = cv2.warpPerspective(frame, M_hold, im_dims['hold_pic'])
                board_pic = cv2.warpPerspective(frame, M_board, im_dims['board_pic'])
                queue_pic = cv2.warpPerspective(frame, M_queue, im_dims['queue_pic'])
                # Get the hold and queue together.
                hold_and_queue = np.zeros((
                    max(hold_pic.shape[0], queue_pic.shape[0]),
                    hold_pic.shape[1] + queue_pic.shape[1],
                    3), dtype=hold_pic.dtype)
                hold_and_queue[:hold_pic.shape[0], :hold_pic.shape[1]] = hold_pic
                hold_and_queue[:queue_pic.shape[0], hold_pic.shape[1]:hold_pic.shape[1] + queue_pic.shape[1]] = queue_pic
                unchecked_corners = get_corner_clicks(hold_and_queue)
                assert len(unchecked_corners) == 2
                click_min_xy, click_max_xy = get_bounding_coords_from_corners(unchecked_corners[0], unchecked_corners[1])
                clicked_part = hold_and_queue[click_min_xy[1]:click_max_xy[1], click_min_xy[0]:click_max_xy[0]]
                cv2.imshow('clicked', clicked_part)
                cv2.waitKey(0)
                cv2.imwrite('template_images/' + param + '.png', clicked_part)
                params.add(param)
            elif command[0] == 'j':
                frames_to_jump = int(command[1:])
                print(f'jumping ahead {frames_to_jump}')
                for  _ in range(frames_to_jump):
                    ret = template_cap.grab()
                    if not ret:
                        break
            elif command == 'help':
                print('q: quit (you must save first with w)')
                print('set {parameter}: after this, click on the two corners of the image you are saving.')
                print('j {NUM}: jump ahead NUM frames')
            else:
                print('Did not understand the command. Try "help"')

            if not ret:
                print('Reached the end of the template file.')
                return
            ret, frame = template_cap.read()
        print('Exiting.')
        return

    # Main logic
    # Now, let's load in all of the images of pieces
    color_names = ['red', 'orange', 'yellow', 'green', 'cyan', 'blue', 'purple']
    piece_images_big = [cv2.imread('template_images/' + name + '_b.png', cv2.IMREAD_GRAYSCALE) for name in color_names]
    piece_images_little = [cv2.imread('template_images/' + name + '_l.png', cv2.IMREAD_GRAYSCALE) for name in color_names]
    template_masks_big = [im > 80 for im in piece_images_big]
    template_masks_little = [im > 80 for im in piece_images_little]

    # This next one is for grayed-out hold pictures:
    assert not any((p is None for p in piece_images_big))
    assert not any((p is None for p in piece_images_little))

    # Any history needs to be reset in the interrupt 'play' option too
    frame_count = 0
    num_state_changes = 0
    # Queue will never actually be all red, but not set to None because None is for a failed read
    last_queue = ['red', 'red', 'red', 'red', 'red', 'red']
    last_hold = None

    queue_history = []
    hold_history = []
    board_history = []

    presented = None

    global interrupted
    with Listener(on_press=interupt, on_release=None) as listener:  # Create an instance of Listener
        while True:

            if interrupted:
                # interact with stdin
                # want to clear stdin before asking to enter command.
                try:
                    x = input('')
                except EOFError as e:
                    continue

                if len(x) == 0:
                    continue

                # if x[0] == '1':
                #     x = x[1:]

                if x == 'play':
                    # resume play
                    interrupted = False

                    fourcc = cv2.VideoWriter_fourcc(*'mp4v')

                    # NOTE: commented this out.
                    writer = cv2.VideoWriter('last_run_video.mp4', fourcc, fps, res)

                    # Reset history; new game
                    frame_count = 0
                    num_state_changes = 0
                    # Queue will never actually be all red, but not set to None because None is for a failed read
                    last_queue = ['red', 'red', 'red', 'red', 'red', 'red']
                    last_hold = None

                    queue_history = []
                    hold_history = []
                    board_history = []


                elif x == 'exit':
                    print('!exit!')
                    break
                else:
                    # send a command through brain to hands.
                    print('!' + x + '!')

            else:
                # Do a loop of regular tetris play.
                t0 = time.time()
                ret, frame = capture.read()

                cv2.imshow('testing', frame)
                cv2.waitKey(1)
                print('test')

                if writer:
                    writer.write(frame)
                t1 = time.time()
                print(t1 - t0)

                continue # TODO remove once we are ready

                downscale = 4

                # TODO find these coords
                upper_left_hold = 'TODO'
                bottom_right_hold = 'TODO'
                hold_pic = frame[upper_left_hold[0]:bottom_right_hold[0], upper_left_hold[1]:bottom_right_hold[1]]
                hold_pic[::downscale, ::downscale]

                upper_left_board = 'TODO'
                bottom_right_board = 'TODO'
                board_pic = 'TODO'
                board_pic = frame[upper_left_board[0]:bottom_right_board[0], upper_left_board[1]:bottom_right_board[1]]
                board_pic[::downscale, ::downscale]

                # TODO consider having something special for the queue because the front of it is bigger
                upper_right_queue = 'TODO'
                bottom_right_queue = 'TODO'
                queue_pic = frame[upper_left_queue[0]:bottom_right_queue[0], upper_left_queue[1]:bottom_right_queue[1]]
                queue_pic[::downscale, ::downscale]

                # TODO next, I think it's worth checking if that gameboy skin does indeed have an exact particular color for empty space
                # if so, this gets very easy




                if not ret:
                    print('Out of video. Exiting.')
                    return

                frame_count += 1
                # print(f'frame count: {frame_count}')
                gray_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
                cv2.imshow('gray_frame', gray_frame)
                # print('test')

                hold_pic = cv2.warpPerspective(gray_frame, M_hold, im_dims['hold_pic'])
                board_pic = cv2.warpPerspective(gray_frame, M_board, im_dims['board_pic'])
                queue_pic = cv2.warpPerspective(gray_frame, M_queue, im_dims['queue_pic'])

                def get_block_match_idx(mask, templates):
                    # If less than 10% isn't black, assume it's empty
                    if np.sum(mask) / mask.size < .05:
                        return None
                    mismatch_counts = []
                    for template in templates:
                        # Compute where to align the two
                        mask_center = np.median(np.nonzero(mask), axis=1)
                        template_center = np.median(np.nonzero(template), axis=1)
                        template_shift = (mask_center - template_center).astype(np.int)
                        starting_corner = (
                                max(min(template_shift[0], mask.shape[0] - template.shape[0]), 0),
                                max(min(template_shift[1], mask.shape[1] - template.shape[1]), 0)
                                )
                        # Assumes that the image provided is at least as large as the template
                        mismatch = mask.copy()
                        mismatch[
                            starting_corner[0]:starting_corner[0] + template.shape[0],
                            starting_corner[1]:starting_corner[1] + template.shape[1]
                                ] ^= template

                        mismatch = cv2.erode(mismatch.astype(np.uint8) * 255, np.ones((2, 2), np.uint8), borderValue=0) != 0
                        mismatch_counts.append(np.sum(mismatch))
                        # cv2.imshow('temp', template.astype(np.uint8) * 255)
                        # cv2.imshow('mismatch', mismatch.astype(np.uint8) * 255)
                        # cv2.waitKey(0)

                    return np.argmin(mismatch_counts)

                # for ind, mask in enumerate(individual_piece_masks):
                    # cv2.imshow('mask' + str(ind), mask.astype(np.uint8) * 255)
                    # cv2.waitKey(0)

                num_cols = 10
                num_rows = 20
                
                individual_piece_pics = np.array_split(queue_pic, 6, axis=0)
                individual_piece_masks = [get_bw_mask(im, piece_stats) for im in individual_piece_pics]

                queue_mask = get_bw_mask(queue_pic, piece_stats)
                hold_mask = get_bw_mask(hold_pic, piece_stats)
                board_mask = get_bw_mask(board_pic, piece_stats)
                cv2.imshow('hold_mask', hold_mask.astype(np.uint8) * 255)

                board_annotated = show_sections(board_mask, num_rows, num_cols)
                cv2.imshow('board_mask', board_annotated)
                # TODO rm
                queue_annotated = np.vstack([cv2.copyMakeBorder(piece.astype(np.uint8) * 255, 2, 2, 2, 2, cv2.BORDER_CONSTANT, None, 128) for piece in individual_piece_masks])
                queue_annotated = show_sections(queue_mask, 6, 1)
                cv2.imshow('queue_mask', queue_annotated)
                
                # Find the board
                full_threshold = .6 # To be considered filled, a piece must have 60% of its pixels be full

                # Find the first row with all empty spaces, and set all spaces above that to 0
                # This is just to getting confused about the piece coming down on top.
                board = np.array([
                    [np.sum(board_pic_cell) / board_pic_cell.size > full_threshold for board_pic_cell in np.array_split(board_pic_row, num_cols, axis=1)]
                        for board_pic_row in np.array_split(board_mask, num_rows, axis=0)
                    ])

                found_empty_row = False
                for row_idx in reversed(range(num_rows)):
                    if found_empty_row:
                        board[row_idx, :] = 0
                    elif np.all(board[row_idx, :] == 0):
                        found_empty_row = True

                # Find the queue
                queue_idxs = [get_block_match_idx(individual_piece_masks[0], template_masks_big)]
                for im in individual_piece_masks[1:]:
                    queue_idxs.append(get_block_match_idx(im, template_masks_little))

                queue = [color_names[idx] if idx is not None else None for idx in queue_idxs]

                # Find the hold
                hold_block_idx = get_block_match_idx(hold_mask, template_masks_big)
                hold = color_names[hold_block_idx] if hold_block_idx != None else None

                # If the first 4 elements of the queue are what we expect based on 
                queue_speculative = last_queue[:3]

                stability_number = 2
                # Check stability
                if len(queue_history) >= stability_number:
                    queue_stable = all(all(a == b for a, b in zip(queue, past_queue)) for past_queue in queue_history)
                    queue_history.append(queue)
                    queue_history.pop(0)
                else:
                    queue_history.append(queue)
                    queue_stable = False

                if len(hold_history) >= stability_number:
                    hold_stable = all((hold == past_hold for past_hold in hold_history))
                    hold_history.append(hold)
                    hold_history.pop(0)
                else:
                    hold_history.append(hold)
                    hold_stable = False

                if len(board_history) >= stability_number:
                    board_stable = all((np.all(board == past_board) for past_board in board_history))
                    board_history.append(board)
                    board_history.pop(0)
                else:
                    board_history.append(board)
                    board_stable = False
                
                pictures_stable = hold_stable and board_stable and queue_stable

                if pictures_stable:
                    # print('histories')
                    # print(hold_change_history)
                    # print(board_change_history)
                    # print(queue_change_history)
                    cv2.imshow('hold_stable', hold_mask.astype(np.uint8) * 255)
                    cv2.imshow('board_stable', board_annotated)
                    cv2.imshow('queue_stable', queue_annotated)
                    # print('stable')

                    # Only just swapped on the first hold, because otherwise the other action will be performed
                    just_swapped = hold is not None and last_hold is None

                    # Check if the queue is valid
                    if all((q != None for q in queue)) and all((q != None for q in last_queue)):

                        assert len(queue) == len(last_queue)
                        # Sanity check: queue should all be populated.
                        # If it's all populated, then the queue changed if anything in the queue changed.
                        assert(all((q != None for q in queue)))
                        assert(all((q != None for q in last_queue)))
                        # The queue changed if it shifted. But we might have gotten
                        # something wrong, so exact equality isn't required.
                        majority_count = 4
                        queue_changed = sum([a != b for a, b in zip(last_queue, queue)]) >= majority_count
                        # queue_changed = any([a != b for a, b in zip(queue, last_queue)])
                        if queue_changed:
                            # TODO this should be logged somehow
                            # if any(a != b for a, b in zip(last_queue[1:], queue[:5])):
                            #     print('whack')
                            #     print(last_queue)
                            #     print(queue)
                            #     cv2.waitKey(0)
                            presented = last_queue[0]

                        if queue_changed:
                            # Something changed, so it's time to update

                            if num_state_changes >= 1:
                                print('presented')
                                print(presented[0])
                                print('queue')
                                print(''.join((q[0] for q in queue)))
                                print('board')
                                expanded_board = np.repeat(board, im_dims['board_pic'][1] / board.shape[0], axis=0)
                                expanded_board = np.repeat(expanded_board, im_dims['board_pic'][0] / board.shape[1], axis=1)
                                cv2.imshow('board', expanded_board.astype(np.uint8) * 255)
                                for row_idx in range(board.shape[0]):
                                    for col_idx in range(board.shape[1]):
                                        print('x' if board[row_idx, col_idx] else '.', end='')
                                    print()
                                print('in_hold')
                                print(hold[0] if hold != None else '.')
                                print('just_swapped')
                                print('true' if just_swapped else 'false', flush=True)

                            num_state_changes += 1
                            # END queue changed or just swapped
                            last_hold = hold
                        last_queue = queue
                        # END queue valid
                    # END picture stable
                last_board_mask = board_mask
                last_hold_mask = hold_mask
                last_queue_mask = queue_mask
                cv2.waitKey(1)
                # END main loop

    capture.release()
    if writer:
        writer.release()
    

if __name__ == '__main__':
    main()
