import pygame
from checkers.constants import WIDTH, HEIGHT, SQUARE_SIZE, RED, WHITE, BLACK, BLUE
from checkers.game import Game
import socket

HOST = "127.0.0.1"  # The server's hostname or IP address
PORT = 2137  # The port used by the server

FPS = 60
WIN = pygame.display.set_mode((WIDTH, HEIGHT))

pygame.display.set_caption('Checkers')


def get_row_col_from_mouse(pos):
    x, y = pos
    row = y // SQUARE_SIZE
    col = x // SQUARE_SIZE
    return row, col


def main():

    run = True
    gameStarted = False
    clock = pygame.time.Clock()

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((HOST, PORT))
        while run:
            clock.tick(FPS)

            if gameStarted == False:
                data = s.recv(1024)
                if data.decode() == "RED":
                    game = Game(WIN, RED, s)
                    game.update()
                    data = s.recv(1024)
                    game.analizeMessage(data.decode())

                else:
                    game = Game(WIN, WHITE, s)
                gameStarted = True

            else:
                if game.winner() != None:
                    print(game.winner())
                    run = False

                for event in pygame.event.get():
                    if event.type == pygame.QUIT:
                        run = False
                    if event.type == pygame.MOUSEBUTTONDOWN and game.turn == game.my_color:
                        pos = pygame.mouse.get_pos()
                        row, col = get_row_col_from_mouse(pos)
                        # if game.turn == RED:
                        game.select(row, col)

            game.update()

        pygame.quit()


main()
