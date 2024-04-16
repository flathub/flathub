import typing as T

from ...language import *
from .. import OutputFormat
from .xml_emitter import XmlEmitter


class XmlOutput(OutputFormat):
    def emit(self, ui: UI) -> str:
        xml = XmlEmitter()
        self._emit_ui(ui, xml)
        return xml.result

    def _emit_ui(self, ui: UI, xml: XmlEmitter):
        xml.start_tag("interface")

        self._emit_gtk_directive(ui.gtk_decl, xml)

        for x in ui.contents:
            if isinstance(x, Template):
                self._emit_template(x, xml)
            elif isinstance(x, Object):
                self._emit_object(x, xml)
            elif isinstance(x, Menu):
                self._emit_menu(x, xml)
            else:
                raise CompilerBugError()

        xml.end_tag()

    def _emit_gtk_directive(self, gtk: GtkDirective, xml: XmlEmitter):
        xml.put_self_closing("requires", lib="gtk", version=gtk.gir_namespace.version)

    def _emit_template(self, template: Template, xml: XmlEmitter):
        xml.start_tag(
            "template", **{"class": template.gir_class}, parent=template.parent_type
        )
        self._emit_object_or_template(template, xml)
        xml.end_tag()

    def _emit_object(self, obj: Object, xml: XmlEmitter):
        xml.start_tag(
            "object",
            **{"class": obj.class_name},
            id=obj.id,
        )
        self._emit_object_or_template(obj, xml)
        xml.end_tag()

    def _emit_object_or_template(
        self, obj: T.Union[Object, Template, ExtListItemFactory], xml: XmlEmitter
    ):
        for child in obj.content.children:
            if isinstance(child, Property):
                self._emit_property(child, xml)
            elif isinstance(child, Signal):
                self._emit_signal(child, xml)
            elif isinstance(child, Child):
                self._emit_child(child, xml)
            else:
                self._emit_extensions(child, xml)

        # List action widgets
        action_widgets = obj.action_widgets
        if action_widgets:
            xml.start_tag("action-widgets")
            for action_widget in action_widgets:
                xml.start_tag(
                    "action-widget",
                    response=action_widget.response_id,
                    default=action_widget.is_default or None,
                )
                xml.put_text(action_widget.widget_id)
                xml.end_tag()
            xml.end_tag()

    def _emit_menu(self, menu: Menu, xml: XmlEmitter):
        xml.start_tag(menu.tag, id=menu.id)
        for child in menu.items:
            if isinstance(child, Menu):
                self._emit_menu(child, xml)
            elif isinstance(child, MenuAttribute):
                xml.start_tag(
                    "attribute",
                    name=child.name,
                    **self._translated_string_attrs(child.value.child),
                )
                xml.put_text(child.value.string)
                xml.end_tag()
            else:
                raise CompilerBugError()
        xml.end_tag()

    def _emit_property(self, property: Property, xml: XmlEmitter):
        value = property.value

        props: T.Dict[str, T.Optional[str]] = {
            "name": property.name,
        }

        if isinstance(value, Value):
            child = value.child

            if isinstance(child, Translated):
                xml.start_tag(
                    "property", **props, **self._translated_string_attrs(child)
                )
                xml.put_text(child.string)
                xml.end_tag()
            else:
                xml.start_tag("property", **props)
                self._emit_value(value, xml)
                xml.end_tag()

        elif isinstance(value, Binding):
            if simple := value.simple_binding:
                props["bind-source"] = self._object_id(value, simple.source)
                props["bind-property"] = simple.property_name
                flags = []
                if not simple.no_sync_create:
                    flags.append("sync-create")
                if simple.inverted:
                    flags.append("invert-boolean")
                if simple.bidirectional:
                    flags.append("bidirectional")
                props["bind-flags"] = "|".join(flags) or None

                xml.put_self_closing("property", **props)
            else:
                xml.start_tag("binding", **props)
                self._emit_expression(value.expression, xml)
                xml.end_tag()

        elif isinstance(value, ObjectValue):
            xml.start_tag("property", **props)
            self._emit_object(value.object, xml)
            xml.end_tag()

        else:
            raise CompilerBugError()

    def _translated_string_attrs(
        self, translated: T.Optional[T.Union[QuotedLiteral, Translated]]
    ) -> T.Dict[str, T.Optional[str]]:
        if translated is None:
            return {}
        elif isinstance(translated, QuotedLiteral):
            return {}
        else:
            return {"translatable": "true", "context": translated.translate_context}

    def _emit_signal(self, signal: Signal, xml: XmlEmitter):
        name = signal.name
        if signal.detail_name:
            name += "::" + signal.detail_name
        xml.put_self_closing(
            "signal",
            name=name,
            handler=signal.handler,
            swapped=signal.is_swapped or None,
            object=(
                self._object_id(signal, signal.object_id) if signal.object_id else None
            ),
        )

    def _emit_child(self, child: Child, xml: XmlEmitter):
        child_type = internal_child = None
        if child.annotation is not None:
            annotation = child.annotation.child
            if isinstance(annotation, ChildType):
                child_type = annotation.child_type
            elif isinstance(annotation, ChildInternal):
                internal_child = annotation.internal_child
            elif isinstance(annotation, ChildExtension):
                child_type = "action"
            else:
                raise CompilerBugError()

        xml.start_tag("child", type=child_type, internal_child=internal_child)
        self._emit_object(child.object, xml)
        xml.end_tag()

    def _emit_literal(self, literal: Literal, xml: XmlEmitter):
        value = literal.value
        if isinstance(value, IdentLiteral):
            value_type = value.context[ValueTypeCtx].value_type
            if isinstance(value_type, gir.BoolType):
                xml.put_text(value.ident)
            elif isinstance(value_type, gir.Enumeration):
                xml.put_text(str(value_type.members[value.ident].value))
            else:
                xml.put_text(self._object_id(value, value.ident))
        elif isinstance(value, TypeLiteral):
            xml.put_text(value.type_name.glib_type_name)
        else:
            if isinstance(value.value, float) and value.value == int(value.value):
                xml.put_text(int(value.value))
            else:
                xml.put_text(value.value)

    def _emit_value(self, value: Value, xml: XmlEmitter):
        if isinstance(value.child, Literal):
            self._emit_literal(value.child, xml)
        elif isinstance(value.child, Flags):
            xml.put_text(
                "|".join([str(flag.value or flag.name) for flag in value.child.flags])
            )
        elif isinstance(value.child, Translated):
            raise CompilerBugError("translated values must be handled in the parent")
        elif isinstance(value.child, TypeLiteral):
            xml.put_text(value.child.type_name.glib_type_name)
        elif isinstance(value.child, ObjectValue):
            self._emit_object(value.child.object, xml)
        else:
            raise CompilerBugError()

    def _emit_expression(self, expression: Expression, xml: XmlEmitter):
        self._emit_expression_part(expression.last, xml)

    def _emit_expression_part(self, expression: ExprBase, xml: XmlEmitter):
        if isinstance(expression, LiteralExpr):
            self._emit_literal_expr(expression, xml)
        elif isinstance(expression, LookupOp):
            self._emit_lookup_op(expression, xml)
        elif isinstance(expression, Expression):
            self._emit_expression(expression, xml)
        elif isinstance(expression, CastExpr):
            self._emit_cast_expr(expression, xml)
        elif isinstance(expression, ClosureExpr):
            self._emit_closure_expr(expression, xml)
        else:
            raise CompilerBugError()

    def _emit_literal_expr(self, expr: LiteralExpr, xml: XmlEmitter):
        if expr.is_object:
            xml.start_tag("constant")
        else:
            xml.start_tag("constant", type=expr.type)
        self._emit_literal(expr.literal, xml)
        xml.end_tag()

    def _emit_lookup_op(self, expr: LookupOp, xml: XmlEmitter):
        xml.start_tag("lookup", name=expr.property_name, type=expr.lhs.type)
        self._emit_expression_part(expr.lhs, xml)
        xml.end_tag()

    def _emit_cast_expr(self, expr: CastExpr, xml: XmlEmitter):
        self._emit_expression_part(expr.lhs, xml)

    def _emit_closure_expr(self, expr: ClosureExpr, xml: XmlEmitter):
        xml.start_tag("closure", function=expr.closure_name, type=expr.type)
        for arg in expr.args:
            self._emit_expression_part(arg.expr, xml)
        xml.end_tag()

    def _emit_attribute(
        self,
        tag: str,
        attr: str,
        name: str,
        value: T.Union[Value, StringValue],
        xml: XmlEmitter,
    ):
        attrs = {attr: name}

        if isinstance(value.child, Translated):
            xml.start_tag(tag, **attrs, **self._translated_string_attrs(value.child))
            xml.put_text(value.child.string)
            xml.end_tag()
        elif isinstance(value.child, QuotedLiteral):
            xml.start_tag(tag, **attrs)
            xml.put_text(value.child.value)
            xml.end_tag()
        else:
            xml.start_tag(tag, **attrs)
            self._emit_value(value, xml)
            xml.end_tag()

    def _emit_extensions(self, extension, xml: XmlEmitter):
        if isinstance(extension, ExtAccessibility):
            xml.start_tag("accessibility")
            for prop in extension.properties:
                self._emit_attribute(prop.tag_name, "name", prop.name, prop.value, xml)
            xml.end_tag()

        elif isinstance(extension, AdwBreakpointCondition):
            xml.start_tag("condition")
            xml.put_text(extension.condition)
            xml.end_tag()

        elif isinstance(extension, AdwBreakpointSetters):
            for setter in extension.setters:
                attrs = {}

                if isinstance(setter.value.child, Translated):
                    attrs = self._translated_string_attrs(setter.value.child)

                xml.start_tag(
                    "setter",
                    object=self._object_id(setter, setter.object_id),
                    property=setter.property_name,
                    **attrs,
                )
                if isinstance(setter.value.child, Translated):
                    xml.put_text(setter.value.child.string)
                elif (
                    isinstance(setter.value.child, Literal)
                    and isinstance(setter.value.child.value, IdentLiteral)
                    and setter.value.child.value.ident == "null"
                    and setter.context[ScopeCtx].objects.get("null") is None
                ):
                    pass
                else:
                    self._emit_value(setter.value, xml)
                xml.end_tag()

        elif isinstance(extension, Filters):
            xml.start_tag(extension.tokens["tag_name"])
            for prop in extension.children:
                xml.start_tag(prop.tokens["tag_name"])
                xml.put_text(prop.tokens["name"])
                xml.end_tag()
            xml.end_tag()

        elif isinstance(extension, ExtComboBoxItems):
            xml.start_tag("items")
            for prop in extension.children:
                self._emit_attribute("item", "id", prop.name, prop.value, xml)
            xml.end_tag()

        elif isinstance(extension, ExtLayout):
            xml.start_tag("layout")
            for prop in extension.children:
                self._emit_attribute("property", "name", prop.name, prop.value, xml)
            xml.end_tag()

        elif isinstance(extension, ExtAdwMessageDialog):
            xml.start_tag("responses")
            for response in extension.responses:
                xml.start_tag(
                    "response",
                    id=response.id,
                    **self._translated_string_attrs(response.value.child),
                    enabled=None if response.enabled else "false",
                    appearance=response.appearance,
                )
                xml.put_text(response.value.string)
                xml.end_tag()
            xml.end_tag()

        elif isinstance(extension, ExtScaleMarks):
            xml.start_tag("marks")
            for mark in extension.children:
                xml.start_tag(
                    "mark",
                    value=mark.value,
                    position=mark.position,
                    **self._translated_string_attrs(mark.label and mark.label.child),
                )
                if mark.label is not None:
                    xml.put_text(mark.label.string)
                xml.end_tag()
            xml.end_tag()

        elif isinstance(extension, ExtStringListStrings):
            xml.start_tag("items")
            for string in extension.children:
                value = string.child
                xml.start_tag("item", **self._translated_string_attrs(value.child))
                xml.put_text(value.string)
                xml.end_tag()
            xml.end_tag()

        elif isinstance(extension, ExtListItemFactory):
            child_xml = XmlEmitter()
            child_xml.start_tag("interface")
            child_xml.start_tag("template", **{"class": "GtkListItem"})
            self._emit_object_or_template(extension, child_xml)
            child_xml.end_tag()
            child_xml.end_tag()
            xml.start_tag("property", name="bytes")
            xml.put_cdata(child_xml.result)
            xml.end_tag()

        elif isinstance(extension, ExtStyles):
            xml.start_tag("style")
            for style in extension.children:
                xml.put_self_closing("class", name=style.name)
            xml.end_tag()

        elif isinstance(extension, ExtSizeGroupWidgets):
            xml.start_tag("widgets")
            for prop in extension.children:
                xml.put_self_closing("widget", name=prop.name)
            xml.end_tag()

        else:
            raise CompilerBugError()

    def _object_id(self, node: AstNode, id: str) -> str:
        if id == "template" and node.context[ScopeCtx].template is not None:
            return node.context[ScopeCtx].template.gir_class.glib_type_name
        else:
            return id
