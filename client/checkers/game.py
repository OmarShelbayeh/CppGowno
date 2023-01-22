import pygame
from .constants import RED, WHITE, SQUARE_SIZE, BLACK, BLUE, ROWS, COLS
from .board import Board
from .piece import Piece


class Game:
    def __init__(self, win, color, socket):
        self._init()
        self.win = win
        self.my_color = color
        self.socket = socket

    def update(self):
        self.board.draw(self.win)

        self.draw_valid_moves(self.valid_moves)
        pygame.display.update()

    def _init(self):
        self.selected = None
        self.board = Board()
        self.turn = WHITE
        self.valid_moves = {}
        self.selected_row = None
        self.selected_col = None
        self.moved_row = None
        self.moved_col = None

    def reset(self):
        self._init()

    def winner(self):
        return self.board.winner()

    def analizeMessage(self, message):
        data = message.split(",")
        piece = self.board.board[int(data[0])][int(data[1])]

        self.board.board[int(data[0])][int(data[1])] = 0
        self.board.board[int(data[2])][int(data[3])] = Piece(
            int(data[2]), int(data[3]), piece.color)
        for i in range(4, len(data), 2):
            self.board.board[int(data[i])][int(data[i+1])] = 0

        self.selected = 0
        self.valid_moves = {}
        self.update()
        self.change_turn()
        self.update()

    def select(self, row, col):
        if self.selected == None:
            self.valid_moves = {}
        if self.selected:
            self.selected_row = row
            self.selected_col = col
            result = self._move(row, col)
            if not result:
                self.selected = None
                self.select(row, col)
        piece = self.board.get_piece(row, col)
        if piece != 0 and piece.color == self.turn:
            self.selected = piece
            self.valid_moves = self.board.get_valid_moves(piece)
            self.moved_row = row
            self.moved_col = col
            return True

        return False

    def _move(self, row, col):
        piece = self.board.get_piece(row, col)
        if self.selected and piece == 0 and (row, col) in self.valid_moves:
            self.board.move(self.selected, row, col)
            skipped = self.valid_moves[(row, col)]
            self.board.skippedThisTurn = []
            if skipped:
                self.board.remove(skipped)

            message = str(self.moved_row) + "," + str(self.moved_col) + "," + \
                str(self.selected_row) + "," + str(self.selected_col) + ","
            messageSkipped = ""
            for skip in self.board.skippedThisTurn:
                messageSkipped += str(skip[0]) + ',' + str(skip[1]) + ","
            finalMessage = message + messageSkipped
            finalMessage = finalMessage[:-1]

            self.socket.send(finalMessage.encode())
            self.change_turn()
            self.update()
            data = self.socket.recv(1024)
            self.analizeMessage(data.decode())

        else:
            return False

        return True

    def change_turn(self):
        self.valid_moves = {}
        if self.turn == RED:
            self.turn = WHITE
        else:
            self.turn = RED

    def draw_valid_moves(self, moves):
        for move in moves:
            row, col = move
            pygame.draw.circle(self.win, BLUE, (col * SQUARE_SIZE +
                               SQUARE_SIZE // 2, row * SQUARE_SIZE + SQUARE_SIZE // 2), 15)
