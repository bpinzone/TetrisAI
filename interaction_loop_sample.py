# pip install pynput
from pynput.keyboard import Listener

interrupted = True

def interupt(key):
    global interrupted
    if not interrupted:
        interrupted = True

def main():

    i = 0
    global interrupted

    with Listener(on_press=interupt, on_release=None) as listener:  # Create an instance of Listener
        while True:

            if interrupted:
                # interact with stdin
                x = input('Enter command:')

                if x == 'play':
                    # resume play
                    interrupted = False
                else:
                    # send a command through brain to hands.
                    print('You entered:' + x)

            else:
                # Do a loop of regular tetris play.
                print(i)
                i += 1

if __name__ == '__main__':
    main()
