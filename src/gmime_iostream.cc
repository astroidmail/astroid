/* ner: src/gmime_iostream.cc
 *
 * Copyright (c) 2010 Michael Forney
 *
 * This file is a part of ner.
 *
 * ner is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License version 3, as published by the Free
 * Software Foundation.
 *
 * ner is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ner.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gmime_iostream.hh"

GMimeIOStreamBuffer::GMimeIOStreamBuffer(GMimeStream * stream)
    : _stream(stream),
        _buffer(4096)
{
    g_object_ref(stream);

    setg(0, 0, 0);
}

GMimeIOStreamBuffer::~GMimeIOStreamBuffer()
{
    g_object_unref(_stream);
}

std::streambuf::int_type GMimeIOStreamBuffer::underflow()
{
    if (gptr() < egptr())
        return traits_type::to_int_type(*gptr());

    char * data = _buffer.data();

    ssize_t n = g_mime_stream_read(_stream, data, _buffer.size());

    if (g_mime_stream_eos(_stream))
        return traits_type::eof();

    setg(data, data, data + n);

    return traits_type::to_int_type(*gptr());
}

GMimeIOStream::GMimeIOStream(GMimeStream * stream)
    : _buffer(stream)
{
    init(&_buffer);
}

// vim: fdm=syntax fo=croql et sw=4 sts=4 ts=8

