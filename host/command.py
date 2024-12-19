import enum


class Command(enum.Enum):
    SET_TIME = b'1'
    ADD_SITE = b'2'
    GET_OTP = b'3'
    ERASE_DB = b'4'
