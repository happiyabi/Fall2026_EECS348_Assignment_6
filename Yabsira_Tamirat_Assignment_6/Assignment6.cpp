/*
Program: EECS 348 Assignment 6 (revised from Assignment 5)
Description: Solves Sudoku puzzles with a recursive, object-oriented
             depth-first search with backtracking. Finds and writes every
             solution to each puzzle into a single solutions.txt file, or
             reports that none exists.
Inputs: Puzzle files given on the command line, or puzzle1.txt through
        puzzle5.txt by default. Each file holds a 9x9 grid of digits 1-9,
        with an underscore (_) marking a blank cell
Outputs: One file named solutions.txt. For each puzzle file it holds a line
         "Puzzle: x" (x = puzzle file name), then either "No solution found."
         or all solutions (in the input file format) followed by
         "Solutions found: n"
Collaborators: Wyatt Sullivan
Other Sources: Claude (Anthropic GenAI) - assisted with the Assignment 6
               single solutions.txt output revision
Author: Yabsira Tamirat
Creation Date: October 14, 2026
Revised: October 14, 2026 (Modified to output to solutionN.txt files with descriptive error messages)
Revisions: Strict file loading that rejects invalid characters and wrong
           cell counts; detects conflicting given digits; tracks used
           digits per row, column and block with bitmasks for constant-time
           checks; searches the empty cell with the fewest candidates
           first; writes each solution to solutionN.txt; handles file
           creation/opening errors with descriptive console messages;
           added a LoadStatus enum, command-line file arguments, and
           comments on every line
               (the two entries above describe the Assignment 5 version)
Revised by: Yabsira Tamirat
Revision Date: November 12, 2026
Assignment 6 Revisions: all puzzle results go to one solutions.txt file,
           each section starting with a "Puzzle: x" line (replaces the
           per-puzzle solutionN.txt files); the search stops scanning
           early once it finds a cell with only one candidate; a console
           message now explains every rejected puzzle (missing file, bad
           format, conflicting givens); the program reports an error and
           exits with code 1 if solutions.txt cannot be fully written
*/

#include <cstddef>  // std::size_t for indexing the list of puzzle files
#include <fstream>  // std::ifstream for reading and std::ofstream for writing
#include <iostream> // std::cout and std::cerr for output and error reporting
#include <string>   // std::string for file names
#include <vector>   // std::vector for the list of puzzle files

// Result of trying to load a puzzle file
enum class LoadStatus
{
    OK,         // file read and givens are valid
    FILE_ERROR, // file could not be opened
    BAD_FORMAT, // wrong number of cells or an invalid character
    CONFLICT    // given digits break a Sudoku rule
};

// Board: stores the 9x9 grid and tracks which digits are used in each row, column and block
class Board
{
public:
    static const int SIZE = 9;           // grid is 9x9
    static const int BLOCK = 3;          // blocks are 3x3
    static const int ALL_DIGITS = 0x3FE; // bits 1-9 set: every digit available

    Board()
    { // constructor: start with an empty board
        for (int r = 0; r < SIZE; ++r)
        {                                               // loop over rows
            rowUsed[r] = colUsed[r] = blockUsed[r] = 0; // no digits used yet
            for (int c = 0; c < SIZE; ++c)
            {                    // loop over columns
                cells[r][c] = 0; // 0 means the cell is empty
            }
        }
    }

    LoadStatus loadFromFile(const std::string &fileName)
    {                               // read a puzzle file strictly
        std::ifstream in(fileName); // open the file
        if (!in)
            return LoadStatus::FILE_ERROR; // report a missing/unreadable file

        int count = 0;         // number of cells read so far
        bool conflict = false; // set if two givens clash
        char ch;               // current character
        while (in >> ch)
        { // read every non-whitespace character
            if (count == SIZE * SIZE)
                return LoadStatus::BAD_FORMAT; // extra data after 81 cells
            int r = count / SIZE;              // row of this cell
            int c = count % SIZE;              // column of this cell
            if (ch == '_')
            {                    // underscore is a blank cell
                cells[r][c] = 0; // leave it empty
            }
            else if (ch >= '1' && ch <= '9')
            {                     // a given digit
                int v = ch - '0'; // convert character to number
                if (!canPlace(r, c, v))
                    conflict = true; // digit already in row/col/block
                else
                    place(r, c, v); // record the given digit
                cells[r][c] = v;    // keep the digit so the puzzle prints as written
            }
            else
            {                                  // anything else is invalid
                return LoadStatus::BAD_FORMAT; // reject the file instead of guessing
            }
            ++count; // move to the next cell
        }
        if (count != SIZE * SIZE)
            return LoadStatus::BAD_FORMAT;                       // too few cells
        return conflict ? LoadStatus::CONFLICT : LoadStatus::OK; // report the result
    }

    bool canPlace(int row, int col, int value) const
    {                                                 // is value legal at (row, col)?
        int bit = 1 << value;                         // bit representing this digit
        return !(rowUsed[row] & bit) &&               // not already in the row
               !(colUsed[col] & bit) &&               // not already in the column
               !(blockUsed[blockOf(row, col)] & bit); // not already in the block
    }

    void place(int row, int col, int value)
    {                                        // put a digit in a cell
        int bit = 1 << value;                // bit for this digit
        cells[row][col] = value;             // store the digit
        rowUsed[row] |= bit;                 // mark it used in the row
        colUsed[col] |= bit;                 // mark it used in the column
        blockUsed[blockOf(row, col)] |= bit; // mark it used in the block
    }

    void remove(int row, int col)
    {                                         // undo a placement (backtrack)
        int bit = 1 << cells[row][col];       // bit for the digit being removed
        cells[row][col] = 0;                  // empty the cell
        rowUsed[row] &= ~bit;                 // free the digit in the row
        colUsed[col] &= ~bit;                 // free the digit in the column
        blockUsed[blockOf(row, col)] &= ~bit; // free the digit in the block
    }

    int candidateMask(int row, int col) const
    {                                                                          // bits of every digit legal at (row, col)
        int used = rowUsed[row] | colUsed[col] | blockUsed[blockOf(row, col)]; // digits taken
        return ALL_DIGITS & ~used;                                             // remaining digits are candidates
    }

    bool isEmpty(int row, int col) const
    {                                // is the cell blank?
        return cells[row][col] == 0; // 0 means blank
    }

    void print(std::ostream &out) const
    { // print the grid, '_' for blanks
        for (int r = 0; r < SIZE; ++r)
        { // each row
            for (int c = 0; c < SIZE; ++c)
            { // each column
                if (c > 0)
                    out << ' '; // space between cells
                if (cells[r][c] == 0)
                    out << '_'; // blank cell
                else
                    out << cells[r][c]; // filled cell
            }
            out << '\n'; // end of row
        }
    }

private:
    int cells[SIZE][SIZE]; // the grid (0 = empty)
    int rowUsed[SIZE];     // bitmask of digits used in each row
    int colUsed[SIZE];     // bitmask of digits used in each column
    int blockUsed[SIZE];   // bitmask of digits used in each block

    static int blockOf(int row, int col)
    {                                               // index 0-8 of the block holding a cell
        return (row / BLOCK) * BLOCK + col / BLOCK; // block row * 3 + block column
    }
};

// SudokuSolver: recursive backtracking that writes each solution as soon as it is found
class SudokuSolver
{
public:
    SudokuSolver(const Board &puzzle, std::ostream &output) // take a copy of the puzzle and an output stream
        : board(puzzle), out(output), solutionCount(0)
    {
    } // nothing found yet

    int solve()
    {                         // run the search, return number of solutions
        solutionCount = 0;    // reset the counter
        search();             // start the recursion
        return solutionCount; // how many solutions were written
    }

private:
    Board board;       // working copy that gets filled in
    std::ostream &out; // where solutions are written
    int solutionCount; // solutions found so far

    static int countBits(int mask)
    {              // number of candidate digits in a mask
        int n = 0; // running count
        while (mask)
        {                     // until no bits remain
            mask &= mask - 1; // clear the lowest set bit
            ++n;              // count it
        }
        return n; // total bits
    }

    void search()
    {                                    // one level of depth-first search
        int bestRow = -1, bestCol = -1;  // empty cell with the fewest candidates
        int bestMask = 0;                // candidates for that cell
        int bestCount = Board::SIZE + 1; // larger than any possible count

        for (int r = 0; r < Board::SIZE && bestCount > 1; ++r)
        { // scan rows until a one-candidate cell is found (Assignment 6: early stop)
            for (int c = 0; c < Board::SIZE && bestCount > 1; ++c)
            { // and columns; no cell can beat a single candidate
                if (!board.isEmpty(r, c))
                    continue;                         // skip filled cells
                int mask = board.candidateMask(r, c); // legal digits here
                int n = countBits(mask);              // how many there are
                if (n < bestCount)
                {                    // fewer options than the current best
                    bestCount = n;   // remember the count
                    bestMask = mask; // remember the candidates
                    bestRow = r;     // remember the row
                    bestCol = c;     // remember the column
                    if (n == 0)
                        return; // dead end: an earlier choice was wrong
                }
            }
        }

        if (bestRow == -1)
        {                     // no empty cells left: board is solved
            ++solutionCount;  // count this solution
            board.print(out); // print solution in input format
            out << '\n';      // blank line between solutions
            return;           // back up to look for more solutions
        }

        for (int v = 1; v <= Board::SIZE; ++v)
        { // try each digit in order
            if (!(bestMask & (1 << v)))
                continue;                     // skip digits that are not candidates
            board.place(bestRow, bestCol, v); // try the digit
            search();                         // recurse with one more cell filled
            board.remove(bestRow, bestCol);   // backtrack and try the next digit
        }
    }
};

// PuzzleRunner: loads one puzzle file and appends its report to the shared solutions.txt stream
// (Assignment 6 revision: authored with help from Claude, see prologue)
class PuzzleRunner
{
public:
    PuzzleRunner(const std::string &file, std::ostream &output) : fileName(file), out(output) {} // remember file name and shared output stream

    void run() const
    {                                          // load, solve, and write this puzzle's section
        out << "Puzzle: " << fileName << "\n"; // header line naming the puzzle file

        Board puzzle;                                      // empty board to load into
        LoadStatus status = puzzle.loadFromFile(fileName); // read the file

        if (status == LoadStatus::FILE_ERROR)
        {
            std::cerr << "Error: Could not open input puzzle file '" << fileName << "'.\n";
            out << "No solution found.\n";
            return;
        }

        if (status == LoadStatus::BAD_FORMAT)
        {
            std::cerr << "Error: Puzzle file '" << fileName << "' is not a valid 9x9 grid.\n"; // explain the rejection
            out << "No solution found.\n";                                                     // invalid file has no solution
            return;
        }

        if (status == LoadStatus::CONFLICT)
        {
            std::cerr << "Error: Puzzle file '" << fileName << "' has given digits that break a Sudoku rule.\n"; // explain the rejection
            out << "No solution found.\n";                                                                       // conflicting givens cannot be solved
            return;
        }

        SudokuSolver solver(puzzle, out); // solver writes solutions to stream
        int count = solver.solve();       // search for every solution

        if (count == 0)
        {
            out << "No solution found.\n"; // no solution existed
        }
        else
        {
            out << "Solutions found: " << count << "\n"; // summary line
        }
    }

private:
    std::string fileName; // which puzzle file to process
    std::ostream &out;    // shared solutions.txt stream all puzzles write to
};

int main(int argc, char *argv[])
{                                   // entry point
    std::vector<std::string> files; // puzzle files to solve
    for (int i = 1; i < argc; ++i)
        files.push_back(argv[i]); // use files given on the command line
    if (files.empty())
    {                                                                                        // none given: use the assignment's five files
        files = {"puzzle1.txt", "puzzle2.txt", "puzzle3.txt", "puzzle4.txt", "puzzle5.txt"}; // defaults
    }

    std::ofstream out("solutions.txt"); // single output file for every puzzle
    if (!out)
    {                                                                          // could not create the file
        std::cerr << "Error: Could not create output file 'solutions.txt'.\n"; // tell the user why
        return 1;                                                              // stop with an error code
    }

    for (std::size_t i = 0; i < files.size(); ++i)
    { // process each file in order
        if (i > 0)
            out << '\n';                    // blank line separating puzzles
        PuzzleRunner runner(files[i], out); // runner for this file
        runner.run();                       // append its results to solutions.txt
    }
    out.close(); // flush and close solutions.txt
    if (!out)
    {                                                                      // a write or the close failed
        std::cerr << "Error: Could not finish writing 'solutions.txt'.\n"; // tell the user the file may be incomplete
        return 1;                                                          // stop with an error code
    }
    return 0; // success
}