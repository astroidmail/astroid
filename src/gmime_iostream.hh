/* ner: src/gmime_iostream.hh
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

# pragma once

#include <iostream>
#include <vector>
#include <gmime/gmime.h>

/**
 * An IO Stream wrapper around a GMimeStream
 */
class GMimeIOStreamBuffer : public std::streambuf
{
    public:
        GMimeIOStreamBuffer(GMimeStream * stream);
        GMimeIOStreamBuffer(const GMimeIOStreamBuffer &) = delete;
        GMimeIOStreamBuffer & operator=(const GMimeIOStreamBuffer &) = delete;
        virtual ~GMimeIOStreamBuffer();

    protected:
        int_type underflow();

    private:
        GMimeStream * _stream;
        std::vector<char> _buffer;
};

class GMimeIOStream : public std::istream
{
    public:
        GMimeIOStream(GMimeStream * stream);

    private:
        GMimeIOStreamBuffer _buffer;
};


// vim: fdm=syntax fo=croql et sw=4 sts=4 ts=8

