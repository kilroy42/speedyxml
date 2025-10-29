from typing import TypeAlias, Union

XML: TypeAlias = tuple[
    str,
    dict[str, str] | None,
    list[Union['XML', str]],
]

__all__ = [
    'parse',
    'XMLParseException',
    'FLAG_EXPANDEMPTY',
    'FLAG_IGNOREENTITIES',
    'FLAG_RETURNCOMMENTS',
    'FLAG_RETURNPI',
    'TAG_COMMENT',
    'TAG_PI',
]

def parse(xml: str, flags: int = 0) -> XML: ...

class XMLParseException(Exception): ...

FLAG_EXPANDEMPTY: int
FLAG_IGNOREENTITIES: int
FLAG_RETURNCOMMENTS: int
FLAG_RETURNPI: int
TAG_COMMENT: str
TAG_PI: str
