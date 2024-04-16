from ..ast_utils import AstNode, validate
from ..parse_tree import Keyword
from .common import *
from .contexts import ScopeCtx
from .gobject_object import ObjectContent, validate_parent_type
from .types import TypeName


class ExtListItemFactory(AstNode):
    grammar = [UseExact("id", "template"), Optional(TypeName), ObjectContent]

    @property
    def id(self) -> str:
        return "template"

    @property
    def signature(self) -> str:
        return f"template {self.gir_class.full_name}"

    @property
    def type_name(self) -> T.Optional[TypeName]:
        if len(self.children[TypeName]) == 1:
            return self.children[TypeName][0]
        else:
            return None

    @property
    def gir_class(self):
        return self.root.gir.get_type("ListItem", "Gtk")

    @validate("template")
    def container_is_builder_list(self):
        validate_parent_type(
            self,
            "Gtk",
            "BuilderListItemFactory",
            "sub-templates",
        )

    @validate("template")
    def unique_in_parent(self):
        self.validate_unique_in_parent("Duplicate template block")

    @validate()
    def type_is_list_item(self):
        if self.type_name is not None:
            if self.type_name.glib_type_name != "GtkListItem":
                raise CompileError(f"Only Gtk.ListItem is allowed as a type here")

    @validate("template")
    def type_name_upgrade(self):
        if self.type_name is None:
            raise UpgradeWarning(
                "Expected type name after 'template' keyword",
                actions=[
                    CodeAction(
                        "Add ListItem type to template block (introduced in blueprint 0.8.0)",
                        "template ListItem",
                    )
                ],
            )

    @context(ScopeCtx)
    def scope_ctx(self) -> ScopeCtx:
        return ScopeCtx(node=self)

    @validate()
    def unique_ids(self):
        self.context[ScopeCtx].validate_unique_ids()

    @property
    def content(self) -> ObjectContent:
        return self.children[ObjectContent][0]

    @property
    def action_widgets(self):
        """
        The sub-template shouldn't have it`s own actions this is
        just hear to satisfy XmlOutput._emit_object_or_template
        """
        return None
