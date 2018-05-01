/*
 *  Copyright (c) 2007,2009 Cyrille Berger <cberger@cberger.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef KIS_RANDOM_GENERATOR_TEST_H
#define KIS_RANDOM_GENERATOR_TEST_H

#include <QtTest>

class KisRandomGeneratorTest : public QObject
{
    Q_OBJECT


private Q_SLOTS:

    void testEvolution();
    void twoSeeds();
    void twoCalls();
    void testConstantness();

private:

    void twoCalls(quint64 seed);
    void testConstantness(quint64 seed);
    void twoSeeds(quint64 seed1, quint64 seed2);

};

#endif
