#!/usr/bin/python3
from kolibri.utils.cli import initialize

initialize()

import os
import click
import enum
import operator

from datetime import datetime

from kolibri.core.content.models import ChannelMetadata
from kolibri.core.content.models import ContentNode
from kolibri.dist.django.db.models import Q

from kolibri.core.content.utils.import_export_content import get_import_export_data
from kolibri.core.content.utils import paths

class OutputFormat(enum.Enum):
    PLAIN = enum.auto()
    INI = enum.auto()


output_format_str_list = list(map(operator.attrgetter("name"), OutputFormat))


@click.command()
@click.argument("output", type=click.File("w"), default="-")
@click.option(
    "-f",
    "--format",
    "output_format_str",
    type=click.Choice(output_format_str_list, case_sensitive=False),
    default=OutputFormat.PLAIN.name,
    help="Use the specified output format",
)
@click.option(
    "-i",
    "--include-channel",
    "include_channel_ids",
    multiple=True,
    default=[],
    metavar="CHANNEL",
    help="Include CHANNEL in the output",
)
@click.option(
    "-x",
    "--exclude-channel",
    "exclude_channel_ids",
    multiple=True,
    default=[],
    metavar="CHANNEL",
    help="Exclude CHANNEL from the output",
)
@click.option(
    "--pick-list-channel",
    "pick_list_channel_ids",
    multiple=True,
    default=[],
    metavar="CHANNEL",
    help="Use CHANNEL as a pick list",
)
def main(
    output,
    output_format_str,
    pick_list_channel_ids,
    include_channel_ids,
    exclude_channel_ids,
):
    """
    Outputs a list of installed Kolibri channels and content, which can be
    used along with a system like eos-image-builder to replicate the selection
    of Kolibri content from $KOLIBRI_HOME.

    By default, the output will be a plain list of channels and content.
    Alternatively, use --format=ini for an INI format compatible with
    eos-image-builder: <https://github.com/endlessm/eos-image-builder/>.

    The output will include every content node that is available in every
    channel that is installed. It is possible to override this behaviour by
    specifying --include-channel or --exclude-channel.

    To use a channel as a pick list, specify --pick-list-channel. In this
    case, matching content from the pick list channel will be treated as
    available in other channels. This is useful for a situation where Kolibri
    content is curated using a new channel in Kolibri Studio, but in a final
    product you wish to have the content presented in its original channels.
    """

    output_format = OutputFormat[output_format_str.upper()]
    output_writer = OutputWriter.by_output_format(output_format)

    if include_channel_ids:
        channelmetadata_query = ChannelMetadata.objects.filter(
            id__in=include_channel_ids
        )
    else:
        channelmetadata_query = ChannelMetadata.objects.exclude(
            id__in=pick_list_channel_ids + exclude_channel_ids
        )

    if pick_list_channel_ids:
        content_selector = ContentSelector_ByPickList(pick_list_channel_ids)
    else:
        content_selector = ContentSelector_ByAvailable()

    content_by_channel = dict()
    for channelmetadata in channelmetadata_query:
        output_writer.add_content_list(
            content_selector.create_content_list(channelmetadata)
        )

    output_writer.write(output)


class OutputWriter(object):
    def __init__(self):
        self.__content_lists = list()

    def write(self, output):
        raise NotImplementedError()

    @staticmethod
    def by_output_format(output_format):
        if output_format == OutputFormat.INI:
            return OutputWriter_INI()
        elif output_format == OutputFormat.PLAIN:
            return OutputWriter_Plain()
        else:
            raise KeyError(output_format)

    @property
    def content_lists(self):
        return iter(self.__content_lists)

    @property
    def visible_content_lists(self):
        return filter(operator.attrgetter("has_content"), self.content_lists)

    @property
    def filtered_content_lists(self):
        return filter(operator.attrgetter("is_subset"), self.content_lists)

    def add_content_list(self, content_list):
        self.__content_lists.append(content_list)


class OutputWriter_Plain(OutputWriter):
    def write(self, output):
        for content_list in self.visible_content_lists:
            click.echo(
                "{name} v{version} ({id})".format(
                    id=content_list.channel_id,
                    version=content_list.channel_version,
                    name=click.style(content_list.channel_name, bold=True),
                ),
                file=output,
            )
            click.secho(
                "{nodes} content nodes - {required_bytes} bytes".format(
                    nodes=content_list.pick_nodes_count,
                    required_bytes=content_list.required_bytes,
                ),
                dim=True,
                file=output,
            )

            if content_list.is_subset:
                for node in content_list.include_nodes:
                    click.echo(
                        "+ {id} ({title}) [{kind}]".format(
                            id=node.id,
                            title=" / ".join(
                                click.style(breadcrumb, bold=True)
                                for breadcrumb in _node_breadcrumbs(node)
                            ),
                            kind=click.style(node.kind, dim=True),
                        ),
                        file=output,
                    )
                for node in content_list.exclude_nodes:
                    click.echo(
                        "- {id} ({title}) [{kind}]".format(
                            id=node.id,
                            title=" / ".join(
                                click.style(breadcrumb, bold=True)
                                for breadcrumb in _node_breadcrumbs(node)
                            ),
                            kind=click.style(node.kind, dim=True),
                        ),
                        file=output,
                    )
            click.echo(file=output)


class OutputWriter_INI(OutputWriter):
    def write(self, output):
        output.write("# Generated by kolibri-listcontent.py\n")
        output.write("# {datetime}\n".format(datetime=datetime.now()))
        output.write("\n")

        total_required_bytes = sum(cl.required_bytes for cl in self.filtered_content_lists)

        output.write("[kolibri]\n")
        output.write("total_required_bytes = {}\n".format(total_required_bytes))
        output.write("install_channels =\n")
        for content_list in self.visible_content_lists:
            output.write(
                "  # {channel} [{nodes}]\n".format(
                    channel=content_list.channel_name,
                    nodes=content_list.pick_nodes_count,
                )
            )
            output.write("  {channel_id}\n".format(channel_id=content_list.channel_id))
        output.write("\n")

        for content_list in self.filtered_content_lists:
            output.write("[kolibri-{}]\n".format(content_list.channel_id))
            output.write("version = {}\n".format(content_list.channel_version))
            output.write("required_bytes = {}\n".format(content_list.required_bytes))
            self.__write_node_list(
                content_list.include_nodes, "include_node_ids", output
            )
            self.__write_node_list(
                content_list.exclude_nodes, "exclude_node_ids", output
            )
            output.write("\n")

    def __write_node_list(self, nodes, key, output):
        if not nodes:
            return
        output.write("{key} =\n".format(key=key))
        for node in nodes:
            output.write(
                "  # {title} [{kind}]\n".format(
                    title=" / ".join(_node_breadcrumbs(node)), kind=node.kind
                )
            )
            output.write("  {id}\n".format(id=node.id))


class ContentSelector(object):
    def query_contentnodes_for_channel(self, channel_id):
        raise NotImplementedError()

    def create_content_list(self, channelmetadata):
        pick_nodes = self.query_contentnodes_for_channel(channelmetadata.id)
        content_list = ContentList(channelmetadata)
        content_list.select_content(pick_nodes)
        return content_list


class ContentSelector_ByAvailable(ContentSelector):
    def query_contentnodes_for_channel(self, channel_id):
        return ContentNode.objects.filter(
            channel_id=channel_id, available=True
        ).exclude(kind="topic")


class ContentSelector_ByPickList(ContentSelector):
    def __init__(self, pick_list_channel_ids, or_available=True):
        self.__pick_contentnode_query = ContentNode.objects.filter(
            channel_id__in=pick_list_channel_ids
        ).exclude(kind="topic")
        self.__or_available = or_available

    def query_contentnodes_for_channel(self, channel_id):
        # When using a pick list, we will treat content in another channel as
        # available if it has the same content ID as content in the pick list,
        # and it has the same parent content ID as content in the pick list.
        q = Q(
            content_id__in=self.__pick_contentnode_content_ids,
            parent__content_id__in=self.__pick_contentnode_parent_content_ids,
        ) | Q(
            content_id__in=self.__pick_contentnode_content_ids,
            parent__content_id=channel_id,
        )
        if self.__or_available:
            q |= Q(available=True)
        return (
            ContentNode.objects.filter(channel_id=channel_id)
            .filter(q)
            .exclude(kind="topic")
        )

    @property
    def __pick_contentnode_content_ids(self):
        return self.__pick_contentnode_query.values("content_id")

    @property
    def __pick_contentnode_parent_content_ids(self):
        return ContentNode.objects.filter(
            id__in=self.__pick_contentnode_query.values("parent")
        ).values("content_id")


class ContentList(object):
    def __init__(self, channelmetadata):
        self.__channelmetadata = channelmetadata
        self.__pick_nodes = None
        self.__include_nodes = set()
        self.__exclude_nodes = set()
        self.__cached_required_bytes = None

    @property
    def channel_id(self):
        return self.__channelmetadata.id

    @property
    def channel_name(self):
        return self.__channelmetadata.name

    @property
    def channel_version(self):
        return self.__channelmetadata.version

    @property
    def has_content(self):
        return self.__pick_nodes.exists()

    @property
    def is_subset(self):
        return self.has_content and (self.include_nodes or self.exclude_nodes)

    @property
    def pick_nodes_count(self):
        return self.__pick_nodes.count()

    @property
    def include_nodes(self):
        return sorted(self.__include_nodes, key=lambda node: node.lft)

    @property
    def exclude_nodes(self):
        return sorted(self.__exclude_nodes, key=lambda node: node.lft)

    @property
    def required_bytes(self):
        if self.__cached_required_bytes is None:
            database_bytes = self.__get_database_bytes()
            storage_bytes = self.__get_storage_bytes()
            self.__cached_required_bytes = database_bytes + storage_bytes

        return self.__cached_required_bytes

    def __get_database_bytes(self):
        database_path = paths.get_content_database_file_path(self.channel_id)
        return os.stat(database_path).st_size

    def __get_storage_bytes(self):
        node_ids = list(node.id for node in self.include_nodes)
        exclude_node_ids = list(node.id for node in self.exclude_nodes)
        _, _, storage_bytes = get_import_export_data(
            available=None,
            channel_id=self.channel_id,
            node_ids=node_ids,
            exclude_node_ids=exclude_node_ids,
        )
        return storage_bytes

    def select_content(self, pick_nodes):
        self.__pick_nodes = pick_nodes.all()
        self.__include_nodes.clear()
        self.__exclude_nodes.clear()

        if not pick_nodes.exists():
            return

        pick_nodes_queue = [self.__channelmetadata.root]

        while len(pick_nodes_queue) > 0:
            node = pick_nodes_queue.pop(0)

            # TODO: It would be nice if we add nodes to exclude_nodes when
            #       less than half of sibling nodes are missing.

            if node.kind == "topic":
                leaf_nodes = _get_leaf_nodes(node)
                matching_leaf_nodes = set(leaf_nodes).intersection(pick_nodes)
                missing_leaf_nodes = set(leaf_nodes).difference(pick_nodes)
                if len(missing_leaf_nodes) == 0:
                    self.__include_nodes.add(node)
                elif len(matching_leaf_nodes) > 0:
                    pick_nodes_queue.extend(node.children.all())
            elif node in pick_nodes:
                self.__include_nodes.add(node)


def _node_breadcrumbs(node):
    titles = [node.title]
    while node.parent:
        node = node.parent
        if node.content_id != node.channel_id:
            titles.append(node.title)
    return reversed(titles)


def _get_leaf_nodes(node):
    return ContentNode.objects.filter(
        lft__gte=node.lft, lft__lte=node.rght, channel_id=node.channel_id
    ).exclude(kind="topic")


if __name__ == "__main__":
    main()
