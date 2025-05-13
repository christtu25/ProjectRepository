import argparse
import multiprocessing as mp
from typing import List, Tuple
import os

class CellularSimulator:
    def __init__(self, input_file: str, output_file: str, process_count: int = 1):
        self.input_file = input_file
        self.output_file = output_file
        self.process_count = process_count
        self.matrix = []
        self.rows = 0
        self.cols = 0

    def read_input_file(self) -> None:
        """Read and validate the input matrix file."""
        if not os.path.exists(self.input_file):
            raise FileNotFoundError(f"Input file {self.input_file} does not exist")

        with open(self.input_file, 'r') as f:
            self.matrix = [list(line.strip()) for line in f.readlines()]

        self.rows = len(self.matrix)
        self.cols = len(self.matrix[0]) if self.rows > 0 else 0

        # Validate matrix dimensions and contents
        valid_chars = {'O', 'X', 'o', 'x', '.'}
        if not all(len(row) == self.cols for row in self.matrix):
            raise ValueError("Matrix rows have inconsistent lengths")
        if not all(all(char in valid_chars for char in row) for row in self.matrix):
            raise ValueError("Matrix contains invalid characters")

    def write_output_file(self) -> None:
        """Write the final matrix to the output file."""
        os.makedirs(os.path.dirname(os.path.abspath(self.output_file)), exist_ok=True)
        with open(self.output_file, 'w') as f:
            for row in self.matrix:
                f.write(''.join(row) + '\n')

    def get_neighbor_sum(self, row: int, col: int, current_matrix: List[List[str]]) -> int:
        """Calculate the sum of neighbor values for a given cell."""
        neighbor_sum = 0
        directions = [(-1, -1), (-1, 0), (-1, 1), (0, -1), (0, 1), (1, -1), (1, 0), (1, 1)]

        for dx, dy in directions:
            new_row, new_col = row + dx, col + dy
            if 0 <= new_row < self.rows and 0 <= new_col < self.cols:
                cell = current_matrix[new_row][new_col]
                # Calculate neighbor value according to rules
                if cell == 'O':
                    neighbor_sum += 2
                elif cell == 'o':
                    neighbor_sum += 1
                elif cell == 'x':
                    neighbor_sum -= 1  # Make sure we're using subtraction
                elif cell == 'X':
                    neighbor_sum -= 2  # Make sure we're using subtraction
                # Dead cells (.) contribute 0
        return neighbor_sum

    def is_power_of_two(self, n: int) -> bool:
        """Check if a number is a power of 2."""
        return n > 0 and (n & (n - 1)) == 0

    def is_prime(self, n: int) -> bool:
        """Check if a number is prime."""
        n = abs(n)
        if n < 2:
            return False
        for i in range(2, int(n ** 0.5) + 1):
            if n % i == 0:
                return False
        return True

    def process_cell(self, row: int, col: int, current_matrix: List[List[str]]) -> str:
        """Process a single cell according to the rules."""
        cell = current_matrix[row][col]
        neighbor_sum = self.get_neighbor_sum(row, col, current_matrix)

        # Healthy O Cell (O)
        if cell == 'O':
            if self.is_power_of_two(neighbor_sum):  # Dies if sum is power of 2
                return '.'
            if neighbor_sum < 10:  # Weakens if sum < 10
                return 'o'
            return 'O'  # Otherwise stays the same

        # Weakened O Cell (o)
        elif cell == 'o':
            if neighbor_sum <= 0:  # Dies if sum <= 0
                return '.'
            if neighbor_sum >= 8:  # Strengthens if sum >= 8
                return 'O'
            return 'o'  # Otherwise stays the same

        # Dead Cell (.)
        elif cell == '.':
            if self.is_prime(
                    abs(neighbor_sum)) and neighbor_sum <= 0:  # Becomes x if abs(sum) is prime and sum is negative
                return 'x'
            elif self.is_prime(neighbor_sum) and neighbor_sum > 0:  # Becomes o if sum is prime and positive
                return 'o'
            return '.'  # Otherwise stays dead

        # Weakened X Cell (x)
        elif cell == 'x':
            if neighbor_sum >= 1:  # Dies if sum >= 1
                return '.'
            if neighbor_sum <= -8:  # Strengthens if sum <= -8
                return 'X'
            return 'x'  # Otherwise stays the same

        # Healthy X Cell (X)
        elif cell == 'X':
            if self.is_power_of_two(abs(neighbor_sum)):  # Dies if abs(sum) is power of 2
                return '.'
            if neighbor_sum > -10:  # Weakens if sum > -10
                return 'x'
            return 'X'  # Otherwise stays the same

        return cell

    def process_row_range(self, start_row: int, end_row: int, current_matrix: List[List[str]],
                          result_dict: dict) -> None:
        """Process a range of rows and store results in shared dictionary."""
        result = []
        for row in range(start_row, end_row):
            new_row = []
            for col in range(self.cols):  # Use self.cols instead of len(current_matrix[0])
                new_cell = self.process_cell(row, col, current_matrix)
                new_row.append(new_cell)
            result.append(new_row)
        result_dict[start_row] = result

    def simulate_step(self) -> None:
        """Simulate one time step using multiprocessing."""
        # Create a deep copy of current matrix
        current_matrix = [row[:] for row in self.matrix]

        # Set up multiprocessing
        manager = mp.Manager()
        result_dict = manager.dict()

        # Calculate rows per process - ensure at least 1 row per process
        rows_per_process = max(1, self.rows // self.process_count)
        processes = []

        # Create and start processes
        for i in range(min(self.process_count, self.rows)):  # Don't create more processes than rows
            start_row = i * rows_per_process
            end_row = start_row + rows_per_process if i < self.process_count - 1 else self.rows

            if start_row >= self.rows:
                break

            p = mp.Process(
                target=self.process_row_range,
                args=(start_row, end_row, current_matrix, result_dict)
            )
            processes.append(p)
            p.start()

        # Wait for all processes to complete
        for p in processes:
            p.join()

        # Combine results in correct order
        new_matrix = []
        for i in range(0, self.rows, rows_per_process):
            if i in result_dict:
                new_matrix.extend(result_dict[i])

        # Make sure we got all rows back
        if len(new_matrix) != self.rows:
            raise ValueError(f"Expected {self.rows} rows but got {len(new_matrix)}")

        self.matrix = new_matrix

    def run_simulation(self) -> None:
        """Run the complete simulation for 100 time steps."""
        print(f"Project :: RX")

        self.read_input_file()

        # Limit process count based on matrix size
        self.process_count = min(self.process_count, self.rows)

        for step in range(100):
            self.simulate_step()

            # Debug output for first few steps
            if step < 2:
                print(f"\nAfter step {step + 1}:")
                for row in self.matrix:
                    print(''.join(row))

        self.write_output_file()

def main():
    parser = argparse.ArgumentParser(description='Cellular Life Simulator')
    parser.add_argument('-i', required=True, help='Path to input file')
    parser.add_argument('-o', required=True, help='Path to output file')
    parser.add_argument('-p', type=int, default=1, help='Number of processes')

    args = parser.parse_args()

    if args.p < 1:
        parser.error("Number of processes must be positive")

    simulator = CellularSimulator(args.i, args.o, args.p)
    simulator.run_simulation()

if __name__ == "__main__":
    main()