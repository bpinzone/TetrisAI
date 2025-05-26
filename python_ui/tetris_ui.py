#! /usr/bin/python3
# EVERYTHING is Top Left = 0, 0

import pygame
import sys
from enum import Enum, auto
from dataclasses import dataclass
from geometry import Rectangle
from color_grid import ColorGrid, Color

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

k_rect_thickness = 2

color_grid = None

def main():
    global screen
    clock = pygame.time.Clock()
    current_w = WINDOW_WIDTH
    current_h = WINDOW_HEIGHT
    
    while True:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                sys.exit()
            elif event.type == pygame.VIDEORESIZE:
                # Handle window resize
                current_w, current_h = event.size
                screen = pygame.display.set_mode((current_w, current_h), pygame.RESIZABLE)
        
        global color_grid

        header = sys.stdin.readline().strip()
        if header == "done":
            break   
        if header == "main_board:":
            color_grid = ColorGrid(sys.stdin)
        
        draw_screen(screen, Rectangle(left=0, top=0, width=current_w, height=current_h))

        # Update the display
        pygame.display.flip()
        

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
    draw_half(surface, screen_section.get_right_half())

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
    draw_child(draw_junk_count_control, Rectangle(left=18, top=30, width=3, height=1))
    draw_child(draw_team_name, Rectangle(left=7, top=1, width=10, height=2))
    draw_child(draw_presented, Rectangle(left=7, top=4, width=10, height=4))
    draw_child(draw_board, Rectangle(left=7, top=9, width=10, height=20))
    draw_child(draw_junk_place_control, Rectangle(left=7, top=30, width=10, height=1))
    draw_child(draw_stats, Rectangle(left=18, top=1, width=12, height=6))
    draw_child(draw_queue_border, Rectangle(left=18, top=9, width=6, height=20))

    for i in range(6):
        draw_child(draw_queue_item, Rectangle(left=19, top=(10 + i * 3), width=4, height=2))

    draw_child(draw_best_board, Rectangle(left=25, top=8, width=5, height=10))
    draw_child(draw_worst_board, Rectangle(left=25, top=19, width=5, height=10))



def draw_hold(surface: pygame.Surface, screen_section: Rectangle):
    draw_rectangle_outline(surface, screen_section, BLUE)

def draw_junk_queue(surface: pygame.Surface, screen_section: Rectangle):
    draw_rectangle_outline(surface, screen_section, RED)

def draw_junk_count_control(surface: pygame.Surface, screen_section: Rectangle):
    draw_rectangle_outline(surface, screen_section, ORANGE)

def draw_team_name(surface: pygame.Surface, screen_section: Rectangle):
    draw_rectangle_outline(surface, screen_section, BROWN)

def draw_presented(surface: pygame.Surface, screen_section: Rectangle):
    draw_rectangle_outline(surface, screen_section, YELLOW)

def draw_board(surface: pygame.Surface, screen_section: Rectangle):


    for row in range(color_grid.rows):
        for col in range(color_grid.cols):
            cell = color_grid.grid[row][col]
            pygame_rect = Rectangle(col, (color_grid.rows - 1) - row, 1, 1)
            cell_subsection = get_subsection_for_child(screen_section, pygame_rect, color_grid.cols, color_grid.rows)

            if cell.shade.is_filled:
                draw_tetrimino(surface, cell_subsection, get_color(cell.color))
            # else:
            #     draw_rectangle_outline(surface, cell_subsection.as_shrunk_square(0.95), GRAY, 1)

    draw_rectangle_outline(surface, screen_section, WHITE)



def draw_junk_place_control(surface: pygame.Surface, screen_section: Rectangle):
    draw_rectangle_outline(surface, screen_section, ORANGE)

def draw_stats(surface: pygame.Surface, screen_section: Rectangle):
    draw_rectangle_outline(surface, screen_section, RED)

def draw_queue_border(surface: pygame.Surface, screen_section: Rectangle):
    draw_rectangle_outline(surface, screen_section, PINK)

def draw_queue_item(surface: pygame.Surface, screen_section: Rectangle):
    draw_rectangle_outline(surface, screen_section, BLUE)

def draw_best_board(surface: pygame.Surface, screen_section: Rectangle):
    draw_rectangle_outline(surface, screen_section, GREEN)

def draw_worst_board(surface: pygame.Surface, screen_section: Rectangle):
    draw_rectangle_outline(surface, screen_section, RED)


def draw_rectangle_outline(surface, rect: Rectangle, color : tuple[int, int, int], thickness=k_rect_thickness):
    py_rect = pygame.Rect(rect.left, rect.top, rect.width, rect.height)
    pygame.draw.rect(surface, color, py_rect, thickness)

def draw_rectangle(surface, rect: Rectangle, color : tuple[int, int, int]):
    py_rect = pygame.Rect(rect.left, rect.top, rect.width, rect.height)
    pygame.draw.rect(surface, color, py_rect)

def draw_tetrimino(surface, rect: Rectangle, color : tuple[int, int, int]):

    outer_rect = rect
    inner_rect = rect.as_shrunk_square(0.9)

    dimming = 0.7
    outer_color = (color[0] * dimming, color[1] * dimming, color[2] * dimming)
    inner_color = color

    draw_rectangle(surface, outer_rect, outer_color)
    draw_rectangle(surface, inner_rect, inner_color)
if __name__ == "__main__":
    print("Starting Tetris UI")
    main() 
