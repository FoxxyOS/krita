/*
 *  Copyright (c) 2007 Adrian Page <adrian@pagenet.plus.com>
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

uniform sampler1D gradientColors;

uniform vec2 gradientVectorStart;
uniform vec2 normalisedGradientVector;

void main(void)
{
    vec2 v = gl_TexCoord[0].st - gradientVectorStart;

    /* Project the vector onto the normalised gradient vector. */
    float t = dot(v, normalisedGradientVector);

    gl_FragColor = texture1D(gradientColors, t);
}
