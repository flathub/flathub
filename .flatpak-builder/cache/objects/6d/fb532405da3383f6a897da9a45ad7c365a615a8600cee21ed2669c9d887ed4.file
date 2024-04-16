from .adw_breakpoint import (
    AdwBreakpointCondition,
    AdwBreakpointSetter,
    AdwBreakpointSetters,
)
from .adw_message_dialog import ExtAdwMessageDialog
from .attributes import BaseAttribute
from .binding import Binding
from .common import *
from .contexts import ScopeCtx, ValueTypeCtx
from .expression import (
    CastExpr,
    ClosureArg,
    ClosureExpr,
    ExprBase,
    Expression,
    LiteralExpr,
    LookupOp,
)
from .gobject_object import Object, ObjectContent
from .gobject_property import Property
from .gobject_signal import Signal
from .gtk_a11y import ExtAccessibility
from .gtk_combo_box_text import ExtComboBoxItems
from .gtk_file_filter import (
    Filters,
    ext_file_filter_mime_types,
    ext_file_filter_patterns,
    ext_file_filter_suffixes,
)
from .gtk_layout import ExtLayout
from .gtk_list_item_factory import ExtListItemFactory
from .gtk_menu import Menu, MenuAttribute, menu
from .gtk_scale import ExtScaleMarks
from .gtk_size_group import ExtSizeGroupWidgets
from .gtk_string_list import ExtStringListStrings
from .gtk_styles import ExtStyles
from .gtkbuilder_child import Child, ChildExtension, ChildInternal, ChildType
from .gtkbuilder_template import Template
from .imports import GtkDirective, Import
from .types import ClassName
from .ui import UI
from .values import (
    Flag,
    Flags,
    IdentLiteral,
    Literal,
    NumberLiteral,
    ObjectValue,
    QuotedLiteral,
    StringValue,
    Translated,
    TypeLiteral,
    Value,
)

OBJECT_CONTENT_HOOKS.children = [
    Signal,
    Property,
    AdwBreakpointCondition,
    AdwBreakpointSetters,
    ExtAccessibility,
    ExtAdwMessageDialog,
    ExtComboBoxItems,
    ext_file_filter_mime_types,
    ext_file_filter_patterns,
    ext_file_filter_suffixes,
    ExtLayout,
    ExtListItemFactory,
    ExtScaleMarks,
    ExtSizeGroupWidgets,
    ExtStringListStrings,
    ExtStyles,
    Child,
]

LITERAL.children = [Literal]
