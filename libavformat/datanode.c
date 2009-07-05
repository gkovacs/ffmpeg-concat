/*
 * Library to parse and represent ini (and later xml) files
 * Copyright (c) 2009 Geza Kovacs
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "datanode.h"

DataNode *ff_datanode_mkchild(DataNode *o)
{
    DataNode *d = av_malloc(sizeof(*d));
    memset(d, 0, sizeof(*d));
    o->child = d;
    d->parent = o;
    return d;
}

DataNode *ff_datanode_mknext(DataNode *o)
{
    DataNode *d = av_malloc(sizeof(*d));
    memset(d, 0, sizeof(*d));
    o->next = d;
    d->prev = o;
    d->parent = o->parent;
    return d;
}

DataNode *ff_datanode_tree_from_ini(char *p)
{
    char c;
    char *s;
    char e;
    int i, b;
    DataNode *o;
    DataNode *d;
    d = av_malloc(sizeof(*d));
    memset(d, 0, sizeof(*d));
    o = d;
    s = d->name;
    e = 1;
    i = b = 0;
    while ((c = *p++)) {
        if (c == '\n') {
            d = ff_datanode_mknext(d);
            i = b = 0;
            s = d->name;
            e = 1;
            continue;
        }
        if (!e) {
            continue;
        }
        if (c == '#') {
            e = 0;
            continue;
        }
        if (c == '[') {
            if (d->parent) {
                d = d->parent;
            }
            d = ff_datanode_mknext(d);
            i = b = 0;
            s = d->name;
            continue;
        }
        if (c == ']') {
            d = ff_datanode_mkchild(d);
            i = b = 0;
            s = d->name;
            continue;
        }
        if (c == '=') {
            i = b = 0;
            s = d->value;
            continue;
        }
        if (i >= b-1) {
            b += DATANODE_STR_BUFSIZE;
            if (s == d->name) {
                s = av_realloc(s, b);
                d->name = s;
            }
            else if (s == d->value) {
                s = av_realloc(s, b);
                d->value = s;
            }
        }
        s[i++] = c;
        s[i] = 0;
    }
    return o;
}

DataNode *ff_datanode_tree_from_xml(char *p)
{
    // TODO
    return NULL;
}

DataNode *ff_datanode_getlognext(DataNode *d)
{
    if (d->child)
        return d->child;
    if (d->next)
        return d->next;
    while ((d = d->parent)) {
        if (d->next)
            return d->next;
    }
    return NULL;
}

void ff_datanode_filter_names_by_value(DataNode *d, StringList *l, char *v)
{
    if (!d)
        return;
    if (d->value && !strncmp(v, d->value, strlen(v)))
        ff_stringlist_append(l, d->name);
    ff_datanode_filter_names_by_value(ff_datanode_getlognext(d), l, v);
}

void ff_datanode_filter_values_by_name(DataNode *d, StringList *l, char *n)
{
    if (!d)
        return;
    if (d->value && !strncmp(n, d->name, strlen(n)))
        ff_stringlist_append(l, d->value);
    ff_datanode_filter_values_by_name(ff_datanode_getlognext(d), l, n);
}

int ff_datanode_getdepth(DataNode *d)
{
    int i = 0;
    while ((d = d->parent))
        ++i;
    return i;
}

void ff_datanode_visualize(DataNode *d)
{
    int i, depth;
    if (!d)
        return;
    depth = ff_datanode_getdepth(d);
    for (i = 0; i < depth; ++i)
        putchar('>');
    printf("name: ");
    if (d->name)
        printf("%s", d->name);
    putchar('\n');
    for (i = 0; i < depth; ++i)
        putchar('>');
    printf("value: ");
    if (d->value)
        printf("%s", d->value);
    putchar('\n');
    ff_datanode_visualize(ff_datanode_getlognext(d));
}

StringList *ff_stringlist_alloc()
{
    StringList *l = av_malloc(sizeof(*l));
    memset(l, 0, sizeof(*l));
    return l;
}

void ff_stringlist_append(StringList *l, char *str)
{
    while (l->next)
        l = l->next;
    if (l->str) {
        l->next = ff_stringlist_alloc();
        l->next->str = str;
    } else
        l->str = str;
}

void ff_stringlist_print(StringList *l)
{
    if (!l)
        return;
    printf("str: ");
    if (l->str)
        printf("%s", l->str);
    putchar('\n');
    ff_stringlist_print(l->next);
}
