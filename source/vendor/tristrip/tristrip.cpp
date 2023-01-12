/*

Copyright (c) 2007-2009, Python File Format Interface
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.

   * Redistributions in binary form must reproduce the above
     copyright notice, this list of conditions and the following
     disclaimer in the documentation and/or other materials provided
     with the distribution.

   * Neither the name of the Python File Format Interface
     project nor the names of its contributors may be used to endorse
     or promote products derived from this software without specific
     prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

*/

#include "tristrip.hpp"
#include "trianglestripifier.hpp"

std::list<std::deque<int> > stripify(const std::list<std::list<int> > & triangles)
{
	// build mesh
	MeshPtr mesh(new Mesh);
	for (std::list<int> triangle : triangles) {
		std::list<int>::const_iterator vertex = triangle.begin();
		assert(vertex != triangle.end());
		int v0 = *vertex++;
		assert(vertex != triangle.end());
		int v1 = *vertex++;
		assert(vertex != triangle.end());
		int v2 = *vertex++;
		assert(vertex == triangle.end());
		if ((v0 != v1) && (v1 != v2) && (v2 != v0))
			mesh->add_face(v0, v1, v2);
	};
	// stripify the mesh
	TriangleStripifier t(mesh);
	std::list<TriangleStripPtr> strips = t.find_all_strips();
	// extract and return triangle strips
	std::list<std::deque<int> > result;
	for (TriangleStripPtr strip : strips) {
		result.push_back(strip->get_strip());
	};
	return result;
};
