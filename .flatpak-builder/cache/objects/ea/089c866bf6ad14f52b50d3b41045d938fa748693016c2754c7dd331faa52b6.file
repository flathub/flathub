# lsp_enums.py
#
# Copyright 2021 James Westman <james@jwestman.net>
#
# This file is free software; you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation; either version 3 of the
# License, or (at your option) any later version.
#
# This file is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# SPDX-License-Identifier: LGPL-3.0-or-later


import enum
import typing as T
from dataclasses import dataclass

from .errors import *
from .utils import *


class TextDocumentSyncKind(enum.IntEnum):
    None_ = 0
    Full = 1
    Incremental = 2


class CompletionItemTag(enum.IntEnum):
    Deprecated = 1


class InsertTextFormat(enum.IntEnum):
    PlainText = 1
    Snippet = 2


class CompletionItemKind(enum.IntEnum):
    Text = 1
    Method = 2
    Function = 3
    Constructor = 4
    Field = 5
    Variable = 6
    Class = 7
    Interface = 8
    Module = 9
    Property = 10
    Unit = 11
    Value = 12
    Enum = 13
    Keyword = 14
    Snippet = 15
    Color = 16
    File = 17
    Reference = 18
    Folder = 19
    EnumMember = 20
    Constant = 21
    Struct = 22
    Event = 23
    Operator = 24
    TypeParameter = 25


class ErrorCode(enum.IntEnum):
    RequestFailed = -32803


@dataclass
class Completion:
    label: str
    kind: CompletionItemKind
    signature: T.Optional[str] = None
    deprecated: bool = False
    docs: T.Optional[str] = None
    text: T.Optional[str] = None
    snippet: T.Optional[str] = None

    def to_json(self, snippets: bool):
        insert_text = self.text or self.label
        insert_text_format = InsertTextFormat.PlainText
        if snippets and self.snippet:
            insert_text = self.snippet
            insert_text_format = InsertTextFormat.Snippet

        result = {
            "label": self.label,
            "kind": self.kind,
            "tags": [CompletionItemTag.Deprecated] if self.deprecated else None,
            "detail": self.signature,
            "documentation": {
                "kind": "markdown",
                "value": self.docs,
            }
            if self.docs
            else None,
            "deprecated": self.deprecated,
            "insertText": insert_text,
            "insertTextFormat": insert_text_format,
        }
        return {k: v for k, v in result.items() if v is not None}


class SemanticTokenType(enum.IntEnum):
    EnumMember = 0


class DiagnosticSeverity(enum.IntEnum):
    Error = 1
    Warning = 2
    Information = 3
    Hint = 4


@dataclass
class SemanticToken:
    start: int
    end: int
    type: SemanticTokenType
