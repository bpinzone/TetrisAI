#! /usr/bin/python3

import pygame
import sys

# Initialize Pygame
pygame.init()

# Get the screen info
screen_info = pygame.display.Info()
SCREEN_WIDTH = screen_info.current_w
SCREEN_HEIGHT = screen_info.current_h

# Constants
BORDER_WIDTH = 20  # Width of the gray border
FPS = 1  # Limit refresh rate to 1 frame per second

# Layout ratios (all values between 0 and 1, relative to inner space)
GAME_AREA_HEIGHT_RATIO = 0.8  # Height of the game area (board, junk, queue)
STATS_HEIGHT_RATIO = 0.2      # Height of the stats area

# Width ratios for components (relative to inner space)
JUNK_WIDTH_RATIO = 0.05   # Approximately 1/17
BOARD_WIDTH_RATIO = 0.7   # Approximately 10/17
QUEUE_WIDTH_RATIO = 0.2   # Approximately 6/17

# Colors
JUNK_COLOR = (0, 255, 0)
BOARD_COLOR = (255, 255, 255)
QUEUE_COLOR = (0, 0, 255)
STATS_COLOR = (255, 165, 0)    # Orange
WORST_BOARD_COLOR = (255, 0, 0)     # Red section for worst board
BEST_BOARD_COLOR = (255, 192, 203)  # Pink section for best board
DIVIDER_COLOR = (128, 0, 128)
BORDER_COLOR = (128, 128, 128)  # Gray
BLACK = (0, 0, 0)

# Create the resizable screen
screen = pygame.display.set_mode((SCREEN_WIDTH, SCREEN_HEIGHT), pygame.RESIZABLE)
pygame.display.set_caption("Tetris Game UI")

def get_inner_space(outer_start_x, outer_width, window_height):
    """Calculate the inner space dimensions within a border"""
    inner_width = outer_width - (2 * BORDER_WIDTH)
    inner_height = window_height - (2 * BORDER_WIDTH)
    inner_start_x = outer_start_x + BORDER_WIDTH
    inner_start_y = BORDER_WIDTH
    return inner_start_x, inner_start_y, inner_width, inner_height

def junk_component(surface, inner_start_x, inner_start_y, inner_width, inner_height):
    """Calculate and render the junk indicator component"""
    # Calculate dimensions
    component_width = inner_width * JUNK_WIDTH_RATIO
    game_height = inner_height * GAME_AREA_HEIGHT_RATIO
    
    # Calculate x position to maintain proper spacing
    total_game_width = inner_width * (JUNK_WIDTH_RATIO + BOARD_WIDTH_RATIO + QUEUE_WIDTH_RATIO)
    start_x = inner_start_x + (inner_width - total_game_width) / 2
    
    # Create rectangle and render
    rect = pygame.Rect(start_x, inner_start_y, component_width, game_height)
    pygame.draw.rect(surface, JUNK_COLOR, rect)
    return rect

def board_component(surface, inner_start_x, inner_start_y, inner_width, inner_height):
    """Calculate and render the main board component"""
    # Calculate dimensions
    component_width = inner_width * BOARD_WIDTH_RATIO
    game_height = inner_height * GAME_AREA_HEIGHT_RATIO
    
    # Calculate x position to maintain proper spacing
    total_game_width = inner_width * (JUNK_WIDTH_RATIO + BOARD_WIDTH_RATIO + QUEUE_WIDTH_RATIO)
    junk_width = inner_width * JUNK_WIDTH_RATIO
    start_x = inner_start_x + (inner_width - total_game_width) / 2 + junk_width
    
    # Create rectangle and render
    rect = pygame.Rect(start_x, inner_start_y, component_width, game_height)
    pygame.draw.rect(surface, BOARD_COLOR, rect)
    return rect

def queue_component(surface, inner_start_x, inner_start_y, inner_width, inner_height):
    """Calculate and render the queue component"""
    # Calculate dimensions
    component_width = inner_width * QUEUE_WIDTH_RATIO
    game_height = inner_height * GAME_AREA_HEIGHT_RATIO
    
    # Calculate x position to maintain proper spacing
    total_game_width = inner_width * (JUNK_WIDTH_RATIO + BOARD_WIDTH_RATIO + QUEUE_WIDTH_RATIO)
    junk_and_board_width = inner_width * (JUNK_WIDTH_RATIO + BOARD_WIDTH_RATIO)
    start_x = inner_start_x + (inner_width - total_game_width) / 2 + junk_and_board_width
    
    # Create rectangle and render
    rect = pygame.Rect(start_x, inner_start_y, component_width, game_height)
    pygame.draw.rect(surface, QUEUE_COLOR, rect)
    return rect

def stats_component(surface, inner_start_x, inner_start_y, inner_width, inner_height):
    """Calculate and render the stats component (orange section)"""
    # Calculate dimensions
    total_game_width = inner_width * (JUNK_WIDTH_RATIO + BOARD_WIDTH_RATIO + QUEUE_WIDTH_RATIO)
    stats_height = inner_height * STATS_HEIGHT_RATIO
    
    # Calculate position
    start_x = inner_start_x + (inner_width - total_game_width) / 2
    start_y = inner_start_y + inner_height - stats_height - BORDER_WIDTH
    
    # Left half (orange)
    stats_width = total_game_width / 2
    stats_rect = pygame.Rect(start_x, start_y, stats_width, stats_height)
    pygame.draw.rect(surface, STATS_COLOR, stats_rect)
    
    return stats_rect

def worst_board_component(surface, inner_start_x, inner_start_y, inner_width, inner_height):
    """Calculate and render the worst board component (red section)"""
    # Calculate dimensions
    total_game_width = inner_width * (JUNK_WIDTH_RATIO + BOARD_WIDTH_RATIO + QUEUE_WIDTH_RATIO)
    component_height = inner_height * STATS_HEIGHT_RATIO
    
    # Calculate position
    start_x = inner_start_x + (inner_width - total_game_width) / 2
    start_y = inner_start_y + inner_height - component_height - BORDER_WIDTH
    
    # Position after stats section
    stats_width = total_game_width / 2
    board_width = stats_width / 2
    board_x = start_x + stats_width
    
    # Create and render rectangle
    rect = pygame.Rect(board_x, start_y, board_width, component_height)
    pygame.draw.rect(surface, WORST_BOARD_COLOR, rect)
    return rect

def best_board_component(surface, inner_start_x, inner_start_y, inner_width, inner_height):
    """Calculate and render the best board component (pink section)"""
    # Calculate dimensions
    total_game_width = inner_width * (JUNK_WIDTH_RATIO + BOARD_WIDTH_RATIO + QUEUE_WIDTH_RATIO)
    component_height = inner_height * STATS_HEIGHT_RATIO
    
    # Calculate position
    start_x = inner_start_x + (inner_width - total_game_width) / 2
    start_y = inner_start_y + inner_height - component_height - BORDER_WIDTH
    
    # Position after worst board section
    stats_width = total_game_width / 2
    board_width = stats_width / 2
    board_x = start_x + stats_width + board_width
    
    # Create and render rectangle
    rect = pygame.Rect(board_x, start_y, board_width, component_height)
    pygame.draw.rect(surface, BEST_BOARD_COLOR, rect)
    return rect

def draw_dashed_line(surface, color, start_pos, end_pos, width=1, dash_length=10):
    x1, y1 = start_pos
    x2, y2 = end_pos
    dl = dash_length
    
    if (x1 == x2):
        ycoords = [y for y in range(y1, y2, dl if y2 > y1 else -dl)]
        if len(ycoords) % 2:
            ycoords.append(y2)
        for i in range(0, len(ycoords), 2):
            if i + 1 < len(ycoords):
                pygame.draw.line(surface, color,
                               (x1, ycoords[i]), (x1, ycoords[i + 1]), width)
    else:
        # Add horizontal line support if needed
        pass

def draw_half_ui(surface, is_right_half, window_width, window_height):
    # Calculate the outer dimensions
    outer_half_width = window_width // 2
    outer_start_x = outer_half_width if is_right_half else 0
    
    # Draw the border rectangle
    pygame.draw.rect(surface, BORDER_COLOR, 
                    (outer_start_x, 0, outer_half_width, window_height))
    
    # Get inner space dimensions
    inner_start_x, inner_start_y, inner_width, inner_height = get_inner_space(
        outer_start_x, outer_half_width, window_height)
    
    # Calculate and render each component independently
    junk_rect = junk_component(surface, inner_start_x, inner_start_y, inner_width, inner_height)
    board_rect = board_component(surface, inner_start_x, inner_start_y, inner_width, inner_height)
    queue_rect = queue_component(surface, inner_start_x, inner_start_y, inner_width, inner_height)
    stats_rect = stats_component(surface, inner_start_x, inner_start_y, inner_width, inner_height)
    worst_board_rect = worst_board_component(surface, inner_start_x, inner_start_y, inner_width, inner_height)
    best_board_rect = best_board_component(surface, inner_start_x, inner_start_y, inner_width, inner_height)

def main():
    running = True
    clock = pygame.time.Clock()
    
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE:
                    running = False
            elif event.type == pygame.VIDEORESIZE:
                # Handle window resize
                screen = pygame.display.set_mode((event.w, event.h), pygame.RESIZABLE)
        
        # Get current window size
        current_w, current_h = screen.get_size()
        
        # Fill screen with black background
        screen.fill(BLACK)
        
        # Draw UI for both halves
        draw_half_ui(screen, False, current_w, current_h)  # Left half
        draw_half_ui(screen, True, current_w, current_h)   # Right half
        
        # Draw divider line in the middle
        draw_dashed_line(screen, DIVIDER_COLOR, 
                        (current_w // 2, 0), 
                        (current_w // 2, current_h),
                        width=2, dash_length=20)
        
        # Update the display
        pygame.display.flip()
        
        # Limit the frame rate
        clock.tick(FPS)
    
    pygame.quit()
    sys.exit()

if __name__ == "__main__":
    main() 