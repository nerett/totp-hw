import enum


class Command(enum.Enum):
    GET_LIST_OF_SITES_ID = b'0'
    GET_SITE_AUTH_CODE = b'1'
