import serial

from typing import List


class Site:
    def __init__(self, hash_: str, name: str):
        self.hash_: str = hash_
        self.name: str = name


sites: List[Site] = [
    Site('1', 'github'),
    Site('2', 'telegram'),
    Site('3', 'music.yandex'),
    Site('4', 'vk')
]

ser: serial.Serial
try:
    port = '/dev/ttyACM0'
    baudrate = 9600
    ser = serial.Serial(port, baudrate=baudrate)
except serial.SerialException as se:
    exit()


def choose_site(ser: serial.Serial) -> Site:
    print('Choose the site:')

    ser.write(b'0')

    line: bytes = ser.readline()
    hashes: List[str] = line.decode().split(',')

    i: int
    site: Site
    for i, site in enumerate(sites):
        if site.hash_ not in hashes:
            continue
        print(str(i + 1) + '. ' + site.name)

    is_site_chosen: bool = False
    while not is_site_chosen:
        i = int(input('Input number: '))
        i -= 1
        if not 0 <= i <= len(sites) - 1 or sites[i].hash_ not in hashes:
            print('Incorrect number. Try again.')
            continue
        is_site_chosen = True

    site = sites[i]
    print('Chosen site: ' + site.name)

    return site


def get_code(hash_: str) -> str:
    ser.write(b'1')
    ser.write(hash_.encode())
    return ser.readline().decode()


if __name__ == '__main__':
    try:
        site: Site = choose_site(ser)
        code: str = get_code(site.hash_)
        print(code)
    except KeyboardInterrupt as ki:
        pass
    finally:
        if ser.is_open:
            ser.close()
