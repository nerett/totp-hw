DEBUG: bool = False


class Serial:
    port: str = '/dev/ttyACM0'
    baud_rate: int = 9600
    timeout: int = 7


class Database:
    name: str = 'database.csv'
