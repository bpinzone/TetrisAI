import argparse
import cv2
import json
import numpy as np
from scipy import stats
import sys
import time
from datetime import datetime
from pynput.keyboard import Listener, Key
import queue
import threading
import copy

interrupted = True
res = (640, 480)
fps = 60

# NOTE: switch "TV SIZE" setting must be set to 96%
# NOTE: switch "TV SIZE" setting must be set to 96%
# NOTE: switch "TV SIZE" setting must be set to 96%
# NOTE: switch "TV SIZE" setting must be set to 96%
upper_left_board = (35, 243)
lower_right_board = (445, 397)
upper_left_queue = (47, 400)
lower_right_queue = (276, 439)
num_cols = 10
num_rows = 20

piece_colors = {
    'r': np.array([0, 0, 255]),
    'o': np.array([0,165,255]),
    'y': np.array([0, 255, 255]),
    'g': np.array([0, 255, 0]),
    'b': np.array([255, 0, 0]),
    'p': np.array([219,112,147]),
    'c': np.array([255,255,224])
}

debugging_processing_queue = queue.Queue(maxsize = 1000)

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

def show_sections(image_mask, rows, cols):
    return np.vstack(
            [np.hstack(
                [cv2.copyMakeBorder(pic_cell.astype(np.uint8) * 255, 2, 2, 2, 2, cv2.BORDER_CONSTANT, None, 128) for pic_cell in np.array_split(pic_row, cols, axis=1)]
                ) for pic_row in np.array_split(image_mask, rows, axis=0)]
            )

# default_video = 0 # '/dev/video0'
default_video = 1 # '/dev/video0'

threshold_to_not_be_empty = 80
cutoff_to_not_be_stars = 210

writer = None
debug_writer = None

def debug_loop():
    global debug_writer
    global writer
    global upper_left_hold
    global lower_right_hold

    global upper_left_board
    global lower_right_board

    global upper_left_queue
    global lower_right_queue
    global num_rows
    global num_cols

    global piece_colors

    this_thread = threading.currentThread()
    while getattr(this_thread, "keep_going", True):
        frame, board, queue, hold, board_obscured = debugging_processing_queue.get()

        debug_image = frame.copy()
        # fill in the board
        # for np.array_split(pic_row, cols, axis=1):
        upper_left_row, upper_left_col = upper_left_board
        lower_right_row, lower_right_col = lower_right_board

        t_before_modifying_board = time.time()

        rows = []
        for bool_row, pic_row in zip(board, np.array_split(debug_image[upper_left_row:lower_right_row, upper_left_col:lower_right_col], num_rows, axis=0)):
            row = []
            for bool_cell, pic_cell in zip(bool_row, np.array_split(pic_row, num_cols, axis=1)):
                modified_cell = pic_cell
                modified_cell[::2, ::2] = 255 if bool_cell else 0
                # TODO rm
                # modified_cell[0, :] = 255
                # modified_cell[:, 0] = 255
                # modified_cell[-1, :] = 255
                # modified_cell[:, -1] = 255
                row.append(modified_cell)
            row = np.hstack(row)
            rows.append(row)
        modified_board = np.vstack(rows)

        queue_upper_left_row, queue_upper_left_col = upper_left_queue
        queue_lower_right_row, queue_lower_right_col = lower_right_queue

        hold_upper_left_row, hold_upper_left_col = upper_left_hold
        hold_lower_right_row, hold_lower_right_col = lower_right_hold
        t_before_queue_pics = time.time()

        queue_pics = []
        for queue_info, queue_pic in zip(queue, np.array_split(debug_image[queue_upper_left_row:queue_lower_right_row, queue_upper_left_col:queue_lower_right_col], 6, axis=0)):
            modified_pic = queue_pic
            color, is_locked = queue_info
            modified_pic[::2, ::2] = piece_colors[color]
            if is_locked:
                modified_pic = cv2.rectangle(modified_pic, (1, 1), (modified_pic.shape[0] - 1, modified_pic.shape[1] - 1), (0, 255, 255), 1)
            queue_pics.append(modified_pic)
        modified_queue = np.vstack(queue_pics)

        debug_image[queue_upper_left_row:queue_lower_right_row, queue_upper_left_col:queue_lower_right_col] = modified_queue
        debug_image[hold_upper_left_row:hold_lower_right_row:2, hold_upper_left_col:hold_lower_right_col:2] = piece_colors[hold] if hold != None else np.array([0, 0, 0])
        # if board_obscured:
        debug_image = cv2.rectangle(debug_image, (upper_left_board[1], upper_left_board[0]), (lower_right_board[1], lower_right_board[0]), (0, 255, 255), 1)
        debug_image = cv2.rectangle(debug_image, (upper_left_queue[1], upper_left_queue[0]), (lower_right_queue[1], lower_right_queue[0]), (0, 255, 255), 1)
        debug_image = cv2.rectangle(debug_image, (upper_left_hold[1], upper_left_hold[0]), (lower_right_hold[1], lower_right_hold[0]), (0, 255, 255), 1)
        # else:
            # debug_image[upper_left_row:lower_right_row, upper_left_col:lower_right_col] = modified_board




        t_before_debug_display = time.time()
        scale_factor = 2
        # if args.big_debug_image:
            # debug_image = cv2.resize(debug_image, (debug_image.shape[1] * scale_factor, debug_image.shape[0] * scale_factor))
        cv2.imshow('debug image', debug_image)

        cv2.waitKey(1)

        t_before_writes = time.time()

        if writer:
            writer.write(frame)
        if debug_writer:
            debug_writer.write(debug_image)
        t_end = time.time()
        # print(f'Time to after print: {t_after_print - t0}')
        # print(f'After print to before modifying board: {t_before_modifying_board - t_after_print}')
        # print(f'Time to before queue pics: {t_before_queue_pics - t_before_modifying_board}')
        # print(f'Time to before debug display: {t_before_debug_display - t_before_queue_pics}')
        # print(f'Time to before writes: {t_before_writes - t_before_debug_display}')
        # print(f'Time of writes: {t_end - t_before_writes}')
        # print(f'')
        # print(f'Total time in our code: {t_end - t0}')
        # print(f'Total time including read: {t_end - t_before_read}')
        # print(f'')
        # print(f'=======================================')

    # TODO
    if writer:
        writer.release()

    if debug_writer:
        debug_writer.release()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-v', '--video', help='path to video file. Uses webcam otherwise', default=default_video)
    parser.add_argument('-d', '--debug', help='write the debug video', action='store_true')
    parser.add_argument('-o', '--overwrite', help='should videos be overwritten or have a timestamp', action='store_true')
    # parser.add_argument('-d', '--big-debug-image', help='Show a bigger debug image (slow)', action='store_true')
    # parser.add_argument('-t', '--make-template', help='Helps you create a template file. Use --video for the template video and --config for the clicked points in the template.', action='store_true')
    # TODO needed?
    # parser.add_argument('-c', '--config', help='path to the clicks configuration file. Creates a new_config otherwise.')
    args = parser.parse_args()


    debug_thread = threading.Thread(target=debug_loop)
    debug_thread.keep_going = True
    debug_thread.start()

    # Figure out where we are getting our images from:
    capture = cv2.VideoCapture(args.video)
    global writer
    global debug_writer
    global res
    global fps
    global upper_left_hold
    global lower_right_hold

    global upper_left_board
    global lower_right_board

    global upper_left_queue
    global lower_right_queue
    global num_rows
    global num_cols
    if args.video == default_video:
        # Need to run this on Linux, but it fails on Windows
        if sys.platform == "linux" or sys.platform == "linux2":
            assert capture.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc('M','J','P','G'));
        # assert capture.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc('M','J','P','G'));
        assert capture.set(cv2.CAP_PROP_FRAME_WIDTH, res[0])
        assert capture.set(cv2.CAP_PROP_FRAME_HEIGHT, res[1])
        assert capture.set(cv2.CAP_PROP_FPS, int(fps))


    def get_bw_mask(image, check_if_too_bright=False):
        if check_if_too_bright:
            return np.logical_and(image > threshold_to_not_be_empty, image < cutoff_to_not_be_stars)
        else:
            return image > threshold_to_not_be_empty

    def is_board_obscured(board):
        return (board > cutoff_to_not_be_stars).sum() / board.size > .03 # if part of the board is bright, consider it obscured

    colors = ['r', 'o', 'y', 'g', 'b', 'p', 'c']

    hold_templates = [get_bw_mask(cv2.imread('template_images/hold_' + l + '.png', cv2.IMREAD_GRAYSCALE)) for l in colors]

    # TODO: keep bw mask here?
    queue_mask_templates = [[get_bw_mask(cv2.imread('template_images/queue_' + str(i) + '_' + l + '.png', cv2.IMREAD_GRAYSCALE)) for l in colors] for i in range(6)]

    # [
    #     [
    #         cv2.imshow(
    #             'template_images/queue_' + str(i) + '_' + l + '.png',
    #             np.asarray(
    #                 get_bw_mask(
    #                     cv2.imread(
    #                         'template_images/queue_' + str(i) + '_' + l + '.png',
    #                         cv2.IMREAD_GRAYSCALE
    #                     )
    #                 ),
    #                 dtype=np.float
    #             ),
    #         )

    #     for l in colors]
    # for i in range(6)]

    # cv2.waitKey(0)

    # Any history needs to be reset in the interrupt 'play' option too
    frame_count = 0
    queue_last_frame = None
    last_held_block = None
    presented = None

    global piece_colors

    # frames_to_skip = 0

    try:

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

                    if x == 'play':
                        # resume play
                        interrupted = False

                        dt_string = ""
                        if not args.overwrite:
                            dt_string = datetime.now().strftime("_%d-%m-%Y-%H-%M-%S")
                        fourcc = cv2.VideoWriter_fourcc(*'mp4v')
                        if args.video == default_video:
                            writer = cv2.VideoWriter(f'last_run_video{dt_string}.mp4', fourcc, fps, res)
                        # Always write the debug
                        debug_writer = None
                        if args.debug:
                            debug_writer = cv2.VideoWriter(f'last_run_video_debug{dt_string}.mp4', fourcc, fps, res)

                        # TODO this should be a class to avoid duplication of reset code.
                        # Reset history; new game
                        frame_count = 0
                        queue_last_frame = None
                        last_held_block = None
                        presented = None


                    elif x == 'exit':
                        print('!exit!')
                        # screw us
                        raise KeyboardInterrupt
                    else:
                        # send a command through brain to hands.
                        print('!' + x + '!')

                else:
                    # Do a loop of regular tetris play.
                    t_before_read = time.time()
                    ret, frame = capture.read()
                    t0 = time.time()

                    if not ret:
                        print('Out of video. Exiting.')
                        return

                    frame_count += 1

                    # Extract a part of an image
                    def get_segment(image, upper_left, lower_right, downsample=1):
                        return image[
                            upper_left[0]:lower_right[0]:downsample,
                            upper_left[1]:lower_right[1]:downsample,
                        ]

                    upper_left_hold = (49, 201)
                    lower_right_hold = (108, 238)
                    hold_pic = get_segment(frame, upper_left_hold, lower_right_hold)

                    board_pic = get_segment(frame, upper_left_board, lower_right_board)

                    queue_pic = get_segment(frame, upper_left_queue, lower_right_queue)


                    # individual_piece_masks = [get_bw_mask(im, piece_stats) for im in individual_piece_pics]

                    # This was for getting the template images
                    # if frames_to_skip > 0:
                    #     frames_to_skip -= 1
                    #     continue

                    # s = input('Number to skip that many frames, 7 letters: holdcolor + queue colors to save')

                    # try:
                    #     frames_to_skip = int(s)
                    # except ValueError:
                    #     assert len(s) == 7
                    #     cv2.imwrite('template_images/hold_' + s[0] + '.png', hold_pic)
                    #     for i, c in enumerate(s[1:]):
                    #         cv2.imwrite('template_images/queue_' + str(i) + '_' + c + '.png', individual_queue_pics[i])
                    # End getting template images

                    hold_gray = cv2.cvtColor(hold_pic, cv2.COLOR_BGR2GRAY)
                    board_gray = cv2.cvtColor(board_pic, cv2.COLOR_BGR2GRAY)
                    queue_gray = cv2.cvtColor(queue_pic, cv2.COLOR_BGR2GRAY)

                    hold_mask, board_mask, queue_mask = get_bw_mask(hold_gray), get_bw_mask(board_gray, check_if_too_bright=True), get_bw_mask(queue_gray)

                    individual_queue_masks = np.array_split(queue_mask, 6, axis=0)

                    # TODO worth considering if we can make this more robust
                    def get_piece_in_image(image, templates, can_be_empty=False):
                        matches = [np.sum(image == templ) for templ in templates]
                        best_match = np.argmax(matches)
                        # if it can be empty and its closer to all black than any template.
                        if can_be_empty and np.sum(image == 0) > matches[best_match]:
                            return None, False # locked_in is irrelevant for the hold
                        else:
                            # case where its all junk and there are stars everywhere.
                            # TODO perhaps add in a 'is this pretty much guaranteed'
                            # TODO is this even still guaranteed or close to it
                            locked_in = matches[best_match] / image.size > .95
                            return colors[best_match], locked_in

                    hold, _ = get_piece_in_image(hold_mask, hold_templates, True)
                    observed_queue = [get_piece_in_image(queue_mask, queue_idx_templates) for queue_mask, queue_idx_templates in zip(individual_queue_masks, queue_mask_templates)]

                    full_threshold = .5

                    # The Forbidden List Comprehension
                    # Do not attempt to understand
                    # TODO change this to < 10% is black
                    board = np.array([
                        [np.sum(board_pic_cell) / board_pic_cell.size > full_threshold for board_pic_cell in np.array_split(board_pic_row, num_cols, axis=1)]
                            for board_pic_row in np.array_split(board_mask, num_rows, axis=0)
                        ])

                    # Remove all floating tiles on the board
                    board = generate_correct_board(board)

                    board_obscured = is_board_obscured(board_gray)

                    # this will be incorrect technically if we swap a block for the same block, but Jeff doesn't do that
                    just_swapped = hold != last_held_block

                    # observed_queue[...][0] = character
                    # observed_queue[...][1] = locked in
                    all_locked_in = queue_last_frame != None and all([observed_queue[0][1], observed_queue[1][1], queue_last_frame[0][1], queue_last_frame[1][1]])

                    queue_shifted = all_locked_in and (observed_queue[0][0] != queue_last_frame[0][0] or observed_queue[1][0] != queue_last_frame[1][0])

                    if queue_shifted:

                        if presented is None:
                            presented = queue_last_frame[0][0]
                        # Sanity check assert
                        # TODO maybe don't have this risk? just choose one?
                        # queue just shifted, so the first 5 elements of observed queue match with the shifted last queue
                        # TODO is this legal. if so use it elsewhere
                        # for (current_q, current_locked), (last_q, last_locked) in zip(observed_queue[:5], queue_last_frame[1:]):
                        #     if current_locked and last_locked:
                        #         assert current_q == last_q, queue_last_frame + [' '] + observed_queue

                        # last queue if it's locked in otherwise use the current one
                        queue = [
                            last_q if last_q[1] and not current_q[1] else current_q
                            for current_q, last_q in zip(observed_queue[:5], queue_last_frame[1:])
                        ] + [observed_queue[5]]

                        print('presented')
                        print(presented)
                        print('queue')
                        print(''.join((q[0] for q, _ in queue)))
                        print('board')
                        for row_idx in range(board.shape[0]):
                            for col_idx in range(board.shape[1]):
                                print('x' if board[row_idx, col_idx] else '.', end='')
                            print()
                        print('in_hold')
                        print(hold[0] if hold != None else '.')
                        print('just_swapped')
                        print('true' if just_swapped else 'false')
                        print('board_obscured')
                        print('true' if board_obscured else 'false')
                        print('', flush=True)
                        last_held_block = hold # only update when the queue changes
                        presented = queue[0][0]
                    else:
                        # Sanity check assert
                        # TODO maybe don't have this risk? just choose one?
                        if queue_last_frame is not None:
                            # for (current_q, current_locked), (last_q, last_locked) in zip(observed_queue, queue_last_frame):
                            #     if current_locked and last_locked:
                            #         assert current_q == last_q, queue_last_frame + [' '] + observed_queue

                            queue = [last_q if last_q[1] and not current_q[1] else current_q for current_q, last_q in zip(observed_queue, queue_last_frame)]
                        else:
                            queue = observed_queue
                    queue_last_frame = queue # update every frame

                    # Send this info to the debugging thread

                    # TODO: put this back.
                    # TODO: put this back.
                    # TODO: put this back.
                    # TODO: put this back.
                    debugging_processing_queue.put_nowait((copy.deepcopy(frame), copy.deepcopy(board), copy.deepcopy(queue), copy.deepcopy(hold), copy.deepcopy(board_obscured)))

                    # debugging_processing_queue.put((copy.deepcopy(frame), copy.deepcopy(board), copy.deepcopy(queue), copy.deepcopy(hold), copy.deepcopy(board_obscured)))

                    t_after_print = time.time()

    except KeyboardInterrupt:

        capture.release()
        debug_thread.keep_going = False
        # Eliminate the possibility of deadlock by adding something to queue after keep going set to false

        # debugging_processing_queue.put((copy.deepcopy(frame), copy.deepcopy(board), copy.deepcopy(queue), copy.deepcopy(hold), copy.deepcopy(board_obscured)))

        # TODO: put this back.
        # TODO: put this back.
        # TODO: put this back.
        # TODO: put this back.
        debugging_processing_queue.put_nowait((copy.deepcopy(frame), copy.deepcopy(board), copy.deepcopy(queue), copy.deepcopy(hold), copy.deepcopy(board_obscured)))

        debug_thread.join()


# coord is an array. [row, col]
def get_neighbors(coord):

    global num_cols
    global num_rows

    neighbors = []

    for row_diff in range(-1, 2):
        for col_diff in range(-1, 2):

            if row_diff == 0 and col_diff == 0:
                continue
            if coord[0] + row_diff < 0 or coord[0] + row_diff >=  num_rows:
                continue
            if coord[1] + col_diff < 0 or coord[1] + col_diff >= num_cols:
                continue
            neighbors.append((coord[0] + row_diff, coord[1] + col_diff))
    return neighbors

def generate_correct_board(board):

    global num_cols
    global num_rows

    # 0,0 is the top left
    # 0, 1 is the top left but one cell to the right.

    # Things get confirmed BEFORE going into the frontier.
    confirmed_cells = np.zeros_like(board, dtype=bool)

    frontier = [(num_rows - 1, col_x) for col_x in range(0, num_cols) if board[num_rows - 1, col_x]]
    for coord in frontier:
        confirmed_cells[coord] = True

    # Things you considered putting into the frontier at some point.

    # Everything in the frontier will be confirmed.
    # When something comes off the frontier, it gets put into visited set.
    # Nothing that is not filled in gets into the frontier.
    while frontier:

        to_explore = frontier[0]
        frontier.pop(0)

        for neighbor in get_neighbors(to_explore):
            if board[neighbor] and not confirmed_cells[neighbor]:
                frontier.append(neighbor)
                confirmed_cells[neighbor] = True

    return confirmed_cells

if __name__ == '__main__':
    main()
