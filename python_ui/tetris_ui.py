#! /usr/bin/python3
# EVERYTHING is Top Left = 0, 0

# dream and nightmare board display is pretty much done. A bit buggy, sometimes blocks are one cell too high.

import os
os.environ["PYGAME_HIDE_SUPPORT_PROMPT"] = "hide"

import pygame
import sys
from enum import Enum, auto
from dataclasses import dataclass
from geometry import Rectangle
from color_grid import ColorGrid, Color
from collections.abc import Callable

# Initialize Pygame
pygame.init()

# Get the screen info
screen_info = pygame.display.Info()
WINDOW_WIDTH = screen_info.current_w / 2
WINDOW_HEIGHT = screen_info.current_h / 2

# Set up the display with resizable flag
screen = pygame.display.set_mode((WINDOW_WIDTH, WINDOW_HEIGHT), pygame.RESIZABLE)
pygame.display.set_caption("Tetris AI Tournament")

# Colors
WHITE = (255, 255, 255)
BLACK = (0, 0, 0)

BLUE = (0, 0, 255)
PURPLE = (128, 0, 128)
RED = (255, 0, 0)
CYAN = (0, 255, 255)
YELLOW = (255, 255, 0)
ORANGE = (255, 165, 0)
GREEN = (0, 255, 0)

PINK = (255, 192, 203)
BROWN = (165, 42, 42)
GRAY = (128, 128, 128)


def get_color(color: Color) -> tuple[int, int, int]:
    if color == Color.BLUE:
        return BLUE
    if color == Color.PURPLE:
        return PURPLE
    if color == Color.RED:
        return RED
    if color == Color.CYAN:
        return CYAN
    if color == Color.YELLOW:
        return YELLOW
    if color == Color.ORANGE:
        return ORANGE
    if color == Color.GREEN:
        return GREEN
    if color == Color.JUNK:
        return GRAY

k_rect_thickness = 2

class UI_state:
    def __init__(self):
        self.queue_str = None
        self.color_grid = None
        self.hold_str = None
        self.presented_str = None
        self.best_envisioned_board = None
        self.worst_envisioned_board = None

class Button:
    def __init__(self, text: str, text_color: tuple[int, int, int], button_color: tuple[int, int, int], on_click: Callable[[], None]):

        self.screen_section : Rectangle = None

        self.text : str = text
        self.text_color : tuple[int, int, int] = text_color

        self.button_color : tuple[int, int, int] = button_color

        self.on_click : Callable[[], None] = on_click
    
    def set_screen_section(self, screen_section: Rectangle):
        self.screen_section = screen_section
    
    def draw(self, surface: pygame.Surface):

        # background
        draw_tetrimino_cell(surface, self.screen_section, self.button_color)

        # text
        draw_text(surface, self.screen_section, self.text, self.text_color)
    
    def possibly_handle_event(self, event: pygame.event.Event):
        assert event.type == pygame.MOUSEBUTTONDOWN
        if event.button == 1:  # Left click

            mouse_pos = pygame.mouse.get_pos()

            py_rect = self.screen_section.as_pygame_rect()
            if py_rect.collidepoint(mouse_pos):
                self.on_click()

class ManualJunkControl:
    def __init__(self):
        self.send_count = 1
        self.plus_button = Button(text="+", text_color=WHITE, button_color=RED, on_click=self.increase_send_count)
        self.minus_button = Button(text="-", text_color=WHITE, button_color=BLUE, on_click=self.decrease_send_count)

        self.num_places = 10
        self.place_buttons = [Button(text="^", text_color=WHITE, button_color=ORANGE, on_click=lambda position=position: self.send_junk(position))
            for position in range(self.num_places)]
    
    def increase_send_count(self):
        self.send_count += 1
        if self.send_count > 19:
            self.send_count = 19
    
    def decrease_send_count(self):
        self.send_count -= 1
        if self.send_count < 1:
            self.send_count = 1
    
    def send_junk(self, position: int):
        print(f"Junk | Pos {position} | Count {self.send_count}")
    
    def draw(self, surface: pygame.Surface, screen_section: Rectangle):

        ui_units_width = 10 + 1 + 3 # place, ui padding, count control buttons.
        ui_units_height = 1

        for i in range(self.num_places):
            place_subsection = get_subsection_for_child(screen_section, Rectangle(left=i, top=0, width=1, height=1), ui_units_width, ui_units_height)
            self.place_buttons[i].set_screen_section(place_subsection)
            self.place_buttons[i].draw(surface)

        minus_subsection = get_subsection_for_child(screen_section, Rectangle(left=11, top=0, width=1, height=1), ui_units_width, ui_units_height)

        send_count_subsection = get_subsection_for_child(screen_section, Rectangle(left=12, top=0, width=1, height=1), ui_units_width, ui_units_height)

        plus_subsection = get_subsection_for_child(screen_section, Rectangle(left=13, top=0, width=1, height=1), ui_units_width, ui_units_height)

        self.minus_button.set_screen_section(minus_subsection)
        self.plus_button.set_screen_section(plus_subsection)

        self.minus_button.draw(surface)
        self.plus_button.draw(surface)
        draw_text(surface, send_count_subsection, str(self.send_count), WHITE)


ui_state = UI_state()
junk_control = ManualJunkControl()

def main():
    global screen
    current_w = WINDOW_WIDTH
    current_h = WINDOW_HEIGHT

    global ui_state

    # todo: do this automatically with static capabilities or something.
    buttons = [junk_control.plus_button, junk_control.minus_button] + junk_control.place_buttons
    
    while True:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                print("QUIT")
                pygame.quit()
                sys.exit()
            elif event.type == pygame.VIDEORESIZE:
                # Handle window resize
                current_w, current_h = event.size
                screen = pygame.display.set_mode((current_w, current_h), pygame.RESIZABLE)
            elif event.type == pygame.MOUSEBUTTONDOWN:
                for button in buttons:
                    button.possibly_handle_event(event)
        
        line = sys.stdin.readline().strip()
        if line == "done":
            break   

        else:

            def read_expected_header(expected_header: str):
                line = sys.stdin.readline().strip()
                if line != expected_header:
                    raise ValueError(f"Expected {expected_header} but got {line}")

            # header
            if line != "ui_frame:":
                raise ValueError(f"Expected ui_frame: but got {line}")
            
            # queue
            read_expected_header("queue:")
            ui_state.queue_str = sys.stdin.readline().strip()
            if len(ui_state.queue_str) != 6:
                raise ValueError(f"Expected 6 tetrimino names in queue, but got {len(ui_state.queue_str)}")
            
            # hold
            read_expected_header("hold:")
            ui_state.hold_str = sys.stdin.readline().strip()
            if len(ui_state.hold_str) != 1:
                raise ValueError(f"Expected 1 tetrimino name in hold, but got {len(ui_state.hold_str)}")

            # presented
            read_expected_header("presented:")
            ui_state.presented_str = sys.stdin.readline().strip()
            if len(ui_state.presented_str) != 1:
                raise ValueError(f"Expected 1 tetrimino name in presented, but got {len(ui_state.presented_str)}")

            # main_board
            read_expected_header("main_board:")
            ui_state.color_grid = ColorGrid(sys.stdin)

            # best_envisioned_board
            read_expected_header("best_envisioned_board:")
            ui_state.best_envisioned_board = ColorGrid(sys.stdin)

            # worst_envisioned_board
            read_expected_header("worst_envisioned_board:")
            ui_state.worst_envisioned_board = ColorGrid(sys.stdin)
        
        draw_screen(screen, Rectangle(left=0, top=0, width=current_w, height=current_h))

        # Update the display
        pygame.display.flip()

        print("ui_complete")
        sys.stdout.flush()
        
        

def get_subsection_for_child(parent_screen_section: Rectangle, child_intraParentPosition: Rectangle, parent_units_width: int, parent_units_height: int):
    """ The parent renders itself inside parent_screen_section.
    This function produces the subsection that the child should render itself in.
    child_intraParentPosition specifies the child's position and size in the parent's coordinate system.
    parent_units_width and parent_units_height specify how many units the parent is divided into."""

    pixels_per_parent_unit_width = parent_screen_section.width // parent_units_width
    pixels_per_parent_unit_height = parent_screen_section.height // parent_units_height

    child_screen_rect_left = parent_screen_section.left + child_intraParentPosition.left * pixels_per_parent_unit_width
    child_screen_rect_top = parent_screen_section.top + child_intraParentPosition.top * pixels_per_parent_unit_height
    child_screen_rect_width = child_intraParentPosition.width * pixels_per_parent_unit_width
    child_screen_rect_height = child_intraParentPosition.height * pixels_per_parent_unit_height
    return Rectangle(child_screen_rect_left, child_screen_rect_top, child_screen_rect_width, child_screen_rect_height)



def draw_screen(surface: pygame.Surface, screen_section: Rectangle):
    surface.fill(BLACK)
    draw_half(surface, screen_section.get_left_half())
    # draw_half(surface, screen_section.get_right_half())

def draw_half(surface: pygame.Surface, screen_section: Rectangle):
    within_margin = screen_section.as_shrunk_square(0.9)
    draw_ui(surface, within_margin)

def draw_ui(surface: pygame.Surface, screen_section: Rectangle):

    draw_rectangle_outline(surface, screen_section, WHITE)

    ui_units_width = 32
    ui_units_width = 32

    def draw_child(draw_func, child_rect: Rectangle):
        draw_func(surface, get_subsection_for_child(screen_section, child_rect, ui_units_width, ui_units_width))

    draw_child(draw_hold, Rectangle(left=2, top=6, width=4, height=2))
    draw_child(draw_junk_queue, Rectangle(left=3, top=9, width=1, height=20))
    # draw_child(draw_junk_count_control, Rectangle(left=18, top=30, width=3, height=1))
    draw_child(draw_team_name, Rectangle(left=7, top=1, width=10, height=2))
    draw_child(draw_presented, Rectangle(left=7, top=4, width=10, height=4))
    draw_child(draw_board, Rectangle(left=7, top=9, width=10, height=20))
    # draw_child(draw_junk_place_control, Rectangle(left=7, top=30, width=10, height=1))
    draw_child(draw_manual_junk_control, Rectangle(left=7, top=30, width=14, height=1))
    draw_child(draw_stats, Rectangle(left=18, top=1, width=12, height=6))
    draw_child(draw_queue, Rectangle(left=18, top=9, width=6, height=20))
    draw_child(draw_best_board, Rectangle(left=25, top=8, width=5, height=10))
    draw_child(draw_worst_board, Rectangle(left=25, top=19, width=5, height=10))



def draw_hold(surface: pygame.Surface, screen_section: Rectangle):
    draw_rectangle_outline(surface, screen_section, BLUE)

    if ui_state.hold_str != "n":
        draw_tetrimino(surface, screen_section, ui_state.hold_str)

def draw_junk_queue(surface: pygame.Surface, screen_section: Rectangle):
    draw_rectangle_outline(surface, screen_section, RED)

# def draw_junk_count_control(surface: pygame.Surface, screen_section: Rectangle):
#     global junk_control
#     junk_control.draw(surface, screen_section)

def draw_manual_junk_control(surface: pygame.Surface, screen_section: Rectangle):
    global junk_control
    junk_control.draw(surface, screen_section)

def draw_team_name(surface: pygame.Surface, screen_section: Rectangle):
    draw_rectangle_outline(surface, screen_section, BROWN)

def draw_presented(surface: pygame.Surface, screen_section: Rectangle):
    draw_rectangle_outline(surface, screen_section, YELLOW)

    ui_units_width = 10
    ui_units_height = 4

    presented_pos = Rectangle(left=1, top=1, width=4, height=2)
    presented_subsection = get_subsection_for_child(screen_section, presented_pos, ui_units_width, ui_units_height)
    draw_tetrimino(surface, presented_subsection, ui_state.presented_str)

def draw_board(surface: pygame.Surface, screen_section: Rectangle):

    draw_some_board(surface, screen_section, ui_state.color_grid)
    draw_rectangle_outline(surface, screen_section, WHITE)

def draw_some_board(surface: pygame.Surface, screen_section: Rectangle, some_color_grid: ColorGrid):

    num_rows = some_color_grid.rows
    num_cols = some_color_grid.cols

    for row in range(num_rows):
        for col in range(num_cols):
            cell = some_color_grid.grid[row][col]
            pygame_rect = Rectangle(col, (num_rows - 1) - row, 1, 1)
            cell_subsection = get_subsection_for_child(screen_section, pygame_rect, num_cols, num_rows)

            if cell.shade.is_filled:
                draw_tetrimino_cell(surface, cell_subsection, get_color(cell.color))
            # else:
            #     draw_rectangle_outline(surface, cell_subsection.as_shrunk_square(0.95), GRAY, 1)


# def draw_junk_place_control(surface: pygame.Surface, screen_section: Rectangle):
#     draw_rectangle_outline(surface, screen_section, ORANGE)

def draw_stats(surface: pygame.Surface, screen_section: Rectangle):
    draw_rectangle_outline(surface, screen_section, RED)

def draw_queue(surface: pygame.Surface, screen_section: Rectangle):

    draw_rectangle_outline(surface, screen_section, PINK)

    ui_units_width = 6
    ui_units_height = 20

    for i in range(6):
        slot_rect = Rectangle(left=1, top=1 + i * 3, width=4, height=2)
        slot_subsection = get_subsection_for_child(screen_section, slot_rect, ui_units_width, ui_units_height)
        draw_tetrimino(surface, slot_subsection, ui_state.queue_str[i])


def draw_best_board(surface: pygame.Surface, screen_section: Rectangle):

    draw_some_board(surface, screen_section, ui_state.best_envisioned_board)
    draw_rectangle_outline(surface, screen_section, GREEN)

def draw_worst_board(surface: pygame.Surface, screen_section: Rectangle):

    draw_some_board(surface, screen_section, ui_state.worst_envisioned_board)
    draw_rectangle_outline(surface, screen_section, RED)


def draw_rectangle_outline(surface, rect: Rectangle, color : tuple[int, int, int], thickness=k_rect_thickness):
    py_rect = pygame.Rect(rect.left, rect.top, rect.width, rect.height)
    pygame.draw.rect(surface, color, py_rect, thickness)

def draw_rectangle(surface, rect: Rectangle, color : tuple[int, int, int]):
    py_rect = pygame.Rect(rect.left, rect.top, rect.width, rect.height)
    pygame.draw.rect(surface, color, py_rect)

def draw_tetrimino_cell(surface, rect: Rectangle, color : tuple[int, int, int]):

    outer_rect = rect
    inner_rect = rect.as_shrunk_square(0.9)

    dimming = 0.7
    outer_color = (color[0] * dimming, color[1] * dimming, color[2] * dimming)
    inner_color = color

    draw_rectangle(surface, outer_rect, outer_color)
    draw_rectangle(surface, inner_rect, inner_color)

def draw_tetrimino(surface, rect: Rectangle, name: str):

    ui_units_width = 4
    ui_units_height = 2

    if name == "b":
        top="1000"
        bot="1110"
    if name == "p":
        top="0100"
        bot="1110"
    if name == "r":
        top="1100"
        bot="0110"
    if name == "c":
        top="0000"
        bot="1111"
    if name == "y":
        top="0110"
        bot="0110"
    if name == "o":
        top="0010"
        bot="1110"
    if name == "g":
        top="0110"
        bot="1100"
    
    color_enum = Color.from_char(name)
    color_val = get_color(color_enum)
    
    # render top
    for i in range(ui_units_width):
        if top[i] == "1":
            square_subsection = get_subsection_for_child(rect, Rectangle(left=i, top=0, width=1, height=1), ui_units_width, ui_units_height)
            draw_tetrimino_cell(surface, square_subsection, color_val)

    # render bottom.
    for i in range(ui_units_width):
        if bot[i] == "1":
            square_subsection = get_subsection_for_child(rect, Rectangle(left=i, top=1, width=1, height=1), ui_units_width, ui_units_height)
            draw_tetrimino_cell(surface, square_subsection, color_val)

# So far only tried with a single character
def draw_text(surface: pygame.Surface, screen_section: Rectangle, text: str, text_color: tuple[int, int, int]):
    py_screen_section = pygame.Rect(screen_section.left, screen_section.top, screen_section.width, screen_section.height)
    font = pygame.font.SysFont('arial', int(screen_section.height))
    text_surface = font.render(text, True, text_color)
    text_rect = text_surface.get_rect(center=py_screen_section.center)
    surface.blit(text_surface, text_rect)

if __name__ == "__main__":
    # print("Starting Tetris UI")
    main() 
