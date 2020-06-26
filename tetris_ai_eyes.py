import argparse
import cv2
import json
import numpy as np
from scipy import stats
import sys

def get_color_distance_image(image, color):
    diff_image = image - color
    color_distances = np.sum(diff_image * diff_image, axis=2)
    return color_distances

def get_sections_of_subimage(img, closest_color_idx_img, colors, vertical_cells, horizontal_cells, has_black_background):
    annotated_image = img.copy()
    color_indices_by_subsection = np.zeros((vertical_cells, horizontal_cells), dtype=np.int)

    for vertical_idx, vertical_slices in enumerate(zip(
            np.array_split(img, vertical_cells, axis=0),
            np.array_split(closest_color_idx_img, vertical_cells, axis=0)
            )):
        vertical_slice_img, vertical_slice_closest_color = vertical_slices
        for horiz_idx, horiz_slices in enumerate(zip(
                np.array_split(vertical_slice_img, horizontal_cells, axis=1),
                np.array_split(vertical_slice_closest_color, horizontal_cells, axis=1)
                )):
            slice_img, slice_closest_color = horiz_slices
            # cv2.imshow('slice', slice_img)
            # cv2.waitKey(0)
            color_of_frame_subsection = get_color_idx_of_image_subsection(slice_closest_color, colors, has_black_background)
            # annotated_image = cv2.rectangle(slice_img, , (img.shape[1], upper_y), colors[color_of_frame_subsection][0], 5)
            color_indices_by_subsection[vertical_idx, horiz_idx] = color_of_frame_subsection
    # TODO annotate image
    return color_indices_by_subsection# , annotated_image

def get_color_idx_of_image_subsection(closest_color_idx_img, colors, has_black_background, color_thres=0.15):
    color_counts = [np.sum(closest_color_idx_img == i) for i in range(len(colors))]
    # Don't detect empty black pixels
    total_pixels = sum(color_counts)
    if has_black_background:
        color_counts[0] = 0
        color_counts[1] = 0
    most_likely_color_index = np.argmax(color_counts)
    if color_counts[most_likely_color_index] > color_thres:
        return most_likely_color_index
    else:
        return 0

# TODO write a function to clear blocks on the top
def clear_impossible_blocks():
    return

def print_state(board, hold, presented, queue, colors):
    # print(f'hold: {colors[hold][1]}')
    # print(f'presented: {colors[presented][1]}')
    print(f'queue: {[colors[idx][1] for idx in queue.flatten()]}')
    # print('board:')
    # for row_idx in range(board.shape[0]):
    #     for col_idx in range(board.shape[1]):
    #         color_of_space = colors[board[row_idx, col_idx]][1]
    #         print('x' if color_of_space != 'black' else '.', end='')
    #     print()

def add_click(event, x, y, flags, param):
    if event == cv2.EVENT_LBUTTONDOWN:
        param.append((x, y))

def get_color_idx_image(image, colors_array):
    color_comparison = image[:, :, :, np.newaxis] - colors_array[np.newaxis, np.newaxis, :, :]
    color_distances = np.sum(np.square(color_comparison), axis=2)
    best_color_indices = np.argmin(color_distances, axis=2)
    return best_color_indices

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

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-v', '--video', help='path to video file. Uses webcam otherwise', default=0)
    parser.add_argument('-t', '--make-template', help='Helps you create a template file. Use --video for the template video and --config for the clicked points in the template.', action='store_true')
    parser.add_argument('-c', '--config', help='path to the clicks configuration file. Creates a new_config otherwise.')
    args = parser.parse_args()

    # We will transform the images so they are of this size.
    # IF YOU CHANGE THESE YOU MUST REMAKE THE TEMPLATE FILE.
    im_dims = {
            'hold_pic': (125, 140),
            'board_pic': (500, 1000),
            'queue_pic': (125, 450)
            }
    hold_ideal_corners, board_ideal_corners, queue_ideal_corners = (np.array([
        [0, 0],
        [dim[0], 0],
        [dim[0], dim[1]],
        [0, dim[1]],
        ], dtype=np.float32) for dim in (im_dims['hold_pic'], im_dims['board_pic'], im_dims['queue_pic']))

    # Figure out where we are getting our images from: 
    capture = cv2.VideoCapture(args.video)
    if args.video == 0:
        ret = capture.set(cv2.CAP_PROP_FRAME_WIDTH,640)
        ret = capture.set(cv2.CAP_PROP_FRAME_HEIGHT,480)

    # Get the corner points of hold, board, and queue
    if args.config == None:
        ret, first_frame = capture.read()
        assert ret
        clicked_points = get_corner_clicks(first_frame)
        save_file_name = 'last_click_coords.config' if not args.make_template else 'last_template_click_coords.config'
        with open(save_file_name, 'w') as f:
            json.dump(clicked_points, f)
    else:
        with open(args.config) as f:
            clicked_points = json.load(f)
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
            cv2.waitKey(1)
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
                print(click_min_xy)
                print(click_max_xy)
                clicked_part = hold_and_queue[click_min_xy[1]:click_max_xy[1], click_min_xy[0]:click_max_xy[0]]
                cv2.imshow('clicked', clicked_part)
                cv2.waitKey(0)
                print('template_images/' + param + '.png')
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
        exit(1)

    # Now, let's load in all of the images of pieces
    color_names = ['red', 'orange', 'yellow', 'green', 'cyan', 'blue', 'purple']
    piece_images_big = [cv2.imread('template_images/' + name + '_b.png') for name in color_names]
    piece_images_little = [cv2.imread('template_images/' + name + '_l.png') for name in color_names]
    assert not any((p is None for p in piece_images_big))
    assert not any((p is None for p in piece_images_little))
    # for point in clicked_points:
    #     frame_annotated = cv2.circle(first_frame_annotated, tuple(point), 5, (0, 0, 255), 2)

    # Based on webcam of the game
    # colors = [
    #         ((0, 0, 0), 'black'), #Black
    #         ((255, 255, 255), 'white'), #White
    #         ((106, 60, 166), 'red'), # Red
    #         ((171, 212, 187), 'yellow'), # Yellow
    #         ((255, 245, 49), 'cyan'), # Cyan
    #         ((255, 86, 172), 'purple'), # Purple TODO measurement noisy
    #         ((243, 255, 97), 'green'), # Green
    #         ((255, 70, 31), 'blue'), # Blue TODO measurement noisy
    #         ((146, 146, 215), 'orange') # Orange
    #         ]
    # Theoretical
    # colors = [
    #         ((0, 0, 0), 'black'), #Black
    #         ((0, 0, 255), 'red'), # Red
    #         ((0, 255, 255), 'yellow'), # Yellow
    #         ((255, 255, 0), 'cyan'), # Cyan
    #         ((255, 0, 255), 'purple'), # Purple
    #         ((0, 255, 0), 'green'), # Green
    #         ((255, 0, 0), 'blue'), # Blue
    #         ((0, 165, 255), 'orange') # Orange
    #     ]
    # Based on direct measurement of the game pixels
    colors = [
            ((0, 0, 0), 'black'), #Black
            ((60, 40, 240), 'red'), # Red
            ((60, 200, 240), 'yellow'), # Yellow
            ((230, 200, 0), 'cyan'), # Cyan
            ((240, 0, 140), 'purple'), # Purple
            ((70, 240, 60), 'green'), # Green
            ((245, 0, 0), 'blue'), # Blue
            ((30, 110, 240), 'orange') # Orange
        ]
    color_array = np.array([c[0] for c in colors]).T

    frame_count = 0

    num_queue_changes = 0
    last_queue = [None, None, None, None, None, None]
    last_queue_pic = np.zeros((im_dims['queue_pic'][1], im_dims['queue_pic'][0], 3), dtype=np.uint8)
    last_queue_mask = np.zeros(im_dims['queue_pic']) == 0

    # Press something once the "get ready" screen is shown and you can see the queue
    _ = input()

    while True:
        ret, frame = capture.read()
        frame_count += 1
        # print(f'frame count: {frame_count}')

        if not ret:
            return

        hold_pic = cv2.warpPerspective(frame, M_hold, im_dims['hold_pic'])
        board_pic = cv2.warpPerspective(frame, M_board, im_dims['board_pic'])
        queue_pic = cv2.warpPerspective(frame, M_queue, im_dims['queue_pic'])


        def get_color_mask(im, colors, erode=False):
            color_ind_image = get_color_idx_image(im, color_array)
            mask = (color_ind_image != 0).astype(np.uint8) * 255

            if erode:
                mask = cv2.erode(mask, np.ones((10, 10), np.uint8), borderValue=0)
            return mask

        queue_mask = get_color_mask(queue_pic, colors, erode=True)
        queue_pic = cv2.copyTo(queue_pic, mask=queue_mask)
        individual_queue_pics = np.array_split(queue_pic, 6, axis=0)

        mask_diffs = np.sum(last_queue_mask != queue_mask)
        # TODO this is a hardcoded threshold, but there should be a better way
        if mask_diffs < 1000:
            # Only check for changes if the picture is stable

            def check_for_block(image, block_pics):
                matched_images = []
                for block_pic in block_pics:
                    template_has_color = np.array(np.sum(block_pic, axis=2) >= 70, dtype=np.uint8) * 255
                    match_im = cv2.matchTemplate(image, block_pic, cv2.TM_SQDIFF, mask=np.repeat(template_has_color[:, :, np.newaxis], 3, axis=2)) / np.sum(template_has_color != 0)
                    matched_images.append(match_im)
                minimums = [np.min(m) for m in matched_images]
                min_diff_image_ind = np.argmin(minimums)

                # TODO might want to consider a way to make this also be able to check for empty.
                # Right now it assumes a block will be present.

                # TODO judging by the minimums, this method may not be very robust

                # min_val = minimums[min_diff_image_ind]
                # for block_pic, match_pic in zip(block_pics, matched_images):
                #     cv2.imshow('block', block_pic)
                #     cv2.imshow('original', image)
                #     print(np.min(match_pic - min_val))
                #     cv2.imshow('matching', np.exp(-match_pic + min_val))
                #     cv2.waitKey(0)
                #     cv2.destroyAllWindows()

                return min_diff_image_ind

            queue = []
            # The first image in the queue is bigger
            queue.append(color_names[check_for_block(individual_queue_pics[0], piece_images_big)])
            for single_queue_pic in individual_queue_pics[1:]:
                queue.append(color_names[check_for_block(single_queue_pic, piece_images_little)])
            assert len(queue) == len(last_queue)
            if any([a != b for a, b in zip(queue, last_queue)]):
                # If this is the first change, print nothing.
                # We are seeing the board for the first time.
                if num_queue_changes != 0:
                    # If we are about to change the board a second time
                    if num_queue_changes == 1:
                        print(''.join([q[0] for q in last_queue]))
                    print(queue[-1][0])
                cv2.imshow('queue pic', queue_pic)
                cv2.waitKey(1)
            num_queue_changes += 1

        last_queue = queue
        last_queue_pic = queue_pic
        last_queue_mask = queue_mask

        # check_for_block(hold_pic, piece_images_big)
        # check_for_block(individual_queue_pics[0], piece_images_big)

        # # Extract the Board from the Image
        # color_medians = np.array([
        #     [np.median(board_pic_cell, axis=(0, 1)) for board_pic_cell in np.array_split(board_pic_row, im_dims['board_pic'][0], axis=1)]
        #         for board_pic_row in np.array_split(board_pic, im_dims['board_pic'][1], axis=0)
        #     ])

        # best_color_indices = get_color_idx_image(color_medians, color_array)

        # # Debugging image
        # debug_image = np.zeros((1000, 1000, 3), dtype=np.uint8)

        # queue_image_color_inds = get_color_idx_image(queue_pic, color_array)
        # hold_image_color_inds = get_color_idx_image(hold_pic, color_array)

        # # Show the hold
        # hold_anchor = 20, 20
        # observed_hold_colors = color_array.T[hold_image_color_inds]
        # debug_image[
        #         hold_anchor[0]:hold_anchor[0] + hold_pic.shape[0],
        #         hold_anchor[1]:hold_anchor[1] + hold_pic.shape[1]
        #         ] = hold_pic
        # debug_image[
        #         hold_anchor[0] + hold_pic.shape[0]:hold_anchor[0] + hold_pic.shape[0] + observed_hold_colors.shape[0],
        #         hold_anchor[1]:hold_anchor[1] + hold_pic.shape[1]] = observed_hold_colors


        # # observed_queue_colors = color_array.T[queue_image_color_inds]
        # # cv2.imshow('seen_in_queue', observed_queue_colors.astype(np.uint8))
        # # cv2.waitKey(1)

        # # Split the queue into 6 parts and find what's in there
        # # debug_image_coords = 
        # individual_queue_pics = np.array_split(queue_pic, 6, axis=0)
        # # sneak the hold in there
        # individual_queue_pics.append(hold_pic)
        # best_guesses_color = []
        # best_guesses_cov = []
        # for im in individual_queue_pics:
        #     color_ind_image = get_color_idx_image(im, color_array)
        #     color_inds, color_counts = np.unique(color_ind_image, return_counts=True)
        #     # there should always be some black pixels
        #     assert color_inds[0] == 0
        #     black_pixel_count = color_counts[0]
        #     color_counts[0] = 0
        #     best_ind = np.argmax(color_counts)
        #     max_block_color_ind = color_inds[best_ind]
        #     max_block_color_count = color_counts[best_ind]

        #     mask = (color_ind_image != 0).astype(np.uint8)
        #     eroded_mask = cv2.erode(mask, np.ones((10, 10), np.uint8), borderValue=0)
        #     median_color = np.median(im[eroded_mask.astype(np.bool)], axis=0)
        #     # print(median_color)
        #     median_color_array = np.array(median_color)
        #     color_after_erosion = get_color_idx_image(median_color_array[np.newaxis, np.newaxis, :], color_array)[0][0]

        #     if np.sum(color_counts) / black_pixel_count >= .2:
        #         best_guesses_color.append(color_after_erosion)
        #     else:
        #         best_guesses_color.append(0)
        #     # cv2.imshow('queue', im)
        #     # cv2.waitKey()
        #     # cv2.imshow('queue mask', eroded_mask * 255)
        #     # cv2.waitKey()
        #     mask_cov = get_mask_normalized_covariance_matrix(eroded_mask)
        #     # block_idx = get_closest_block_from_mask(mask_cov, ideal_covariances)
        #     # best_guesses_cov.append(block_idx)

        # queue_anchor = 20, 200
        # observed_queue_colors = color_array.T[queue_image_color_inds]
        # debug_image[
        #         queue_anchor[0]:queue_anchor[0] + queue_pic.shape[0],
        #         queue_anchor[1]:queue_anchor[1] + queue_pic.shape[1]
        #         ] = queue_pic
        # debug_image[
        #         queue_anchor[0]:queue_anchor[0] + queue_pic.shape[0],
        #         queue_anchor[1] + queue_pic.shape[1]:queue_anchor[1] + queue_pic.shape[1] + observed_queue_colors.shape[1]
        #         ] = observed_queue_colors


        # cv2.imshow('debug_view', debug_image)
        # cv2.waitKey(0)

        # if last_guesses != best_guesses_color:
        #     print('queue')
        #     print([colors[i][1] for i in best_guesses_color[:6]])
        #     print('hold')
        #     print(colors[best_guesses_color[-1]][1])
        #     print('board')
        #     print(best_color_indices)
        #     last_guesses = best_guesses_color
        # # print('best_guesses_cov')
        # # print([block_ideals[i][1] for i in best_guesses_cov])

    # END MAIN WHILE

    # hold_cov = get_mask_normalized_covariance_matrix(hold_mask)

if __name__ == '__main__':
    main()
