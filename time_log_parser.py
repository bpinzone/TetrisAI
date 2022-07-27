import sys

def main():
    assert len(sys.argv) == 2

    with open(sys.argv[1]) as time_log:

        last_time = None

        for line in time_log:

            this_time = float(line.split(' ')[0])
            print(line, end='')

            if last_time is not None:
                elapsed_ms = round(this_time - last_time, 6)
                if elapsed_ms > 10:
                    print(f"\t (+ {elapsed_ms} ms!!!!!!!!!!)")
                elif elapsed_ms > 5:
                    print(f"\t (+ {elapsed_ms} ms!!!!!)")
                elif elapsed_ms > 1:
                    print(f"\t (+ {elapsed_ms} ms!)")

            last_time = this_time


if __name__ == '__main__':
    main()