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
    # parser.add_argument('-t', '--make-template', help='Helps you create a template file. Use --video for the template video and --config for the clicked points in the template.', action='store_true')
    # TODO needed?
    # parser.add_argument('-c', '--config', help='path to the clicks configuration file. Creates a new_config otherwise.')
    args = parser.parse_args()

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


    # TODO consider grayscale conversion
    # TODO consider searching for > 10% black for a cell to be empty
    def get_bw_mask(image):
        return np.sum(image, axis=2) > 250

    colors = ['r', 'o', 'y', 'g', 'b', 'p', 'c']

    hold_templates = [get_bw_mask(cv2.imread('template_images/hold_' + l + '.png')) for l in colors]

    # TODO: keep bw mask here?
    queue_mask_templates = [[get_bw_mask(cv2.imread('template_images/queue_' + str(i) + '_' + l + '.png')) for l in colors] for i in range(6)]

    # Any history needs to be reset in the interrupt 'play' option too
    frame_count = 0
    queue_last_frame = None
    last_held_block = None
    presented = None


    # frames_to_skip = 0

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

                    fourcc = cv2.VideoWriter_fourcc(*'mp4v')

                    # NOTE: commented this out.
                    writer = cv2.VideoWriter('last_run_video.mp4', fourcc, fps, res)

                    # TODO this should be a class to avoid duplication of reset code.
                    # Reset history; new game
                    frame_count = 0
                    queue_last_frame = None
                    last_held_block = None
                    presented = None


                elif x == 'exit':
                    print('!exit!')
                    break
                else:
                    # send a command through brain to hands.
                    print('!' + x + '!')

            else:
                # Do a loop of regular tetris play.
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

                # upper_left_board = (36, 242)
                upper_left_board = (35, 243)
                # lower_right_board = (446, 398)
                lower_right_board = (445, 397)
                # lower_right_board = (35 + 20 * 21, 243 + 10 * 21)
                board_pic = get_segment(frame, upper_left_board, lower_right_board)

                upper_left_queue = (47, 400)
                lower_right_queue = (276, 439)
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


                hold_mask, board_mask, queue_mask = get_bw_mask(hold_pic), get_bw_mask(board_pic), get_bw_mask(queue_pic)

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
                        locked_in = matches[best_match] / image.size > .95
                        return colors[best_match], locked_in

                hold, _ = get_piece_in_image(hold_mask, hold_templates, True)
                observed_queue = [get_piece_in_image(queue_mask, queue_idx_templates) for queue_mask, queue_idx_templates in zip(individual_queue_masks, queue_mask_templates)]

                num_cols = 10
                num_rows = 20
                full_threshold = .5

                # The Forbidden List Comprehension
                # Do not attempt to understand
                # TODO change this to < 10% is black
                board = np.array([
                    [np.mean(board_pic_cell) > full_threshold for board_pic_cell in np.array_split(board_pic_row, num_cols, axis=1)]
                        for board_pic_row in np.array_split(board_mask, num_rows, axis=0)
                    ])

                # Remove all floating tiles on the board
                found_empty_row = False
                for row_idx in reversed(range(num_rows)):
                    if found_empty_row:
                        board[row_idx, :] = 0
                    elif np.all(board[row_idx, :] == 0):
                        found_empty_row = True

                # TODO everything below this should be gone until the end of the loop


                # this will be incorrect technically if we swap a block for the same block, but Jeff doesn't do that
                just_swapped = hold != last_held_block

                # observed_queue[...][0] = character
                # observed_queue[...][1] = locked in
                queue_shifted = queue_last_frame != None and (observed_queue[0][0] != queue_last_frame[0][0] or observed_queue[1][0] != queue_last_frame[1][0])

                if queue_shifted:
                    presented = queue_last_frame[0]

                    # Sanity check assert
                    # TODO maybe don't have this risk? just choose one?
                    # queue just shifted, so the first 5 elements of observed queue match with the shifted last queue
                    # TODO is this legal. if so use it elsewhere
                    for (current_q, current_locked), (last_q, last_locked) in zip(observed_queue[:5], queue_last_frame[1:]):
                        if current_locked and last_locked:
                            assert current_q == last_q, queue_last_frame + [' '] + observed_queue

                    # last queue if it's locked in otherwise use the current one
                    queue = [last_q if last_q[1] else current_q for current_q, last_q in zip(observed_queue[:5], queue_last_frame[1:])] + [observed_queue[5]]

                    print('presented')
                    print(presented[0])
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
                    print('true' if just_swapped else 'false', flush=True)
                    last_held_block = hold # only update when the queue changes
                else:
                    # Sanity check assert
                    # TODO maybe don't have this risk? just choose one?
                    if queue_last_frame is not None:
                        for (current_q, current_locked), (last_q, last_locked) in zip(observed_queue, queue_last_frame):
                            if current_locked and last_locked:
                                assert current_q == last_q, queue_last_frame + [' '] + observed_queue

                        queue = [last_q if last_q[1] else current_q for current_q, last_q in zip(observed_queue, queue_last_frame)]
                    else:
                        queue = observed_queue
                queue_last_frame = queue # update every frame

                t1 = time.time()
                # print(t1 - t0)

                # Scope: regular tetris play loop

                # TODO create one debug picture to show at the end
                debug_image = frame.copy()
                # fill in the board
                # for np.array_split(pic_row, cols, axis=1):
                upper_left_row, upper_left_col = upper_left_board
                lower_right_row, lower_right_col = lower_right_board

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

                piece_colors = {
                    'r': np.array([0, 0, 255]),
                    'o': np.array([0,165,255]),
                    'y': np.array([0, 255, 255]),
                    'g': np.array([0, 255, 0]),
                    'b': np.array([255, 0, 0]),
                    'p': np.array([219,112,147]),
                    'c': np.array([255,255,224])
                }

                queue_pics = []
                for queue_info, queue_pic in zip(queue, np.array_split(debug_image[queue_upper_left_row:queue_lower_right_row, queue_upper_left_col:queue_lower_right_col], 6, axis=0)):
                    modified_pic = queue_pic
                    color, _ = queue_info
                    modified_pic[::2, ::2] = piece_colors[color]
                    queue_pics.append(modified_pic)
                modified_queue = np.vstack(queue_pics)

                debug_image[upper_left_row:lower_right_row, upper_left_col:lower_right_col] = modified_board
                debug_image[queue_upper_left_row:queue_lower_right_row, queue_upper_left_col:queue_lower_right_col] = modified_queue
                debug_image[hold_upper_left_row:hold_lower_right_row:2, hold_upper_left_col:hold_lower_right_col:2] = piece_colors[hold] if hold != None else np.array([0, 0, 0])




                # cell_height = (lower_right_row - upper_left_row) // num_rows
                # cell_width = (lower_right_col - upper_left_col) // num_cols
                # for row_ind, row in enumerate(board):
                #     for col_ind, cell in enumerate(row):
                #         cell_upper_left_row = upper_left_row + cell_height * row_ind
                #         cell_lower_right_row = cell_upper_left_row + cell_height
                #         cell_upper_left_col = upper_left_col + cell_width * col_ind
                #         cell_lower_right_col = cell_upper_left_col + cell_width
                #         debug_image[cell_upper_left_row:cell_lower_right_row:2, cell_upper_left_col:cell_lower_right_col:2, :] = 255 if cell else 0

                cv2.imshow('debug image', debug_image)
                cv2.waitKey(1)

                if writer:
                    writer.write(frame)

    capture.release()
    if writer:
        writer.release()

if __name__ == '__main__':
    main()
