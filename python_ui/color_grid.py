from enum import Enum, auto
from dataclasses import dataclass
from typing import TextIO, List

class Color(Enum):
    BLUE = auto()
    PURPLE = auto()
    RED = auto()
    CYAN = auto()
    YELLOW = auto()
    ORANGE = auto()
    GREEN = auto()
    JUNK = auto()

    @staticmethod
    def from_char(c: str) -> 'Color':
        color_map = {
            'b': Color.BLUE,
            'p': Color.PURPLE,
            'r': Color.RED,
            'c': Color.CYAN,
            'y': Color.YELLOW,
            'o': Color.ORANGE,
            'g': Color.GREEN,
            'j': Color.JUNK,
        }
        return color_map[c]

@dataclass
class ColorCellShade:
    is_filled: bool = False
    is_ghost: bool = False
    is_about_to_be_cleared: bool = False 

@dataclass
class ColorCell:
    color: Color
    shade: ColorCellShade

    @staticmethod
    def from_data_str(data_str: str) -> 'ColorCell':
        # Parse format: c:COLOR,s:(f:IS_FILLED,g:IS_GHOST,a:IS_ABOUT_TO_BE_CLEARED)
        color_char = data_str[2]  # Get character after 'c:'
        color = Color.from_char(color_char)
        
        # Parse shade values
        f_pos = data_str.find('f:') + 2
        g_pos = data_str.find('g:') + 2
        a_pos = data_str.find('a:') + 2
        
        filled = bool(int(data_str[f_pos]))
        ghost = bool(int(data_str[g_pos]))
        about_to_clear = bool(int(data_str[a_pos]))
        
        return ColorCell(
            color=color,
            shade=ColorCellShade(
                is_filled=filled,
                is_ghost=ghost,
                is_about_to_be_cleared=about_to_clear
            )
        )

class ColorGrid:
    def __init__(self, input_stream: TextIO):
        # Read dimensions
        rows_line = input_stream.readline().strip()
        cols_line = input_stream.readline().strip()
        self.rows = int(rows_line.split(': ')[1])
        self.cols = int(cols_line.split(': ')[1])
        
        # Initialize empty grid with default cells
        default_cell = ColorCell(Color.BLUE, ColorCellShade())
        self.grid: List[List[ColorCell]] = [[default_cell for _ in range(self.cols)] for _ in range(self.rows)]
        
        # Read each line of the format: row:ROW, col:COL, data: DATA
        for row in range(self.rows):
            for col in range(self.cols):
                line = input_stream.readline().strip()

                if not line.strip():
                    continue
                    
                # Parse row and column using string splits
                comma_parts = line.split(',')
                row = int(comma_parts[0].split(':')[1])
                col = int(comma_parts[1].split(':')[1])

                data_parts = line.split('data:')
                data_str = data_parts[1].strip()
                
                # Store the cell
                self.grid[row][col] = ColorCell.from_data_str(data_str)




    



