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

// Based on:
// http://techgame.net/projects/Runeblade/browser/trunk/RBRapier/RBRapier/Tools/Geometry/Analysis/TriangleMesh.py?rev=760

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~ License of original code:
//~
//- The RuneBlade Foundation library is intended to ease some
//- aspects of writing intricate Jabber, XML, and User Interface (wxPython, etc.)
//- applications, while providing the flexibility to modularly change the
//- architecture. Enjoy.
//~
//~ Copyright (C) 2002  TechGame Networks, LLC.
//~
//~ This library is free software; you can redistribute it and/or
//~ modify it under the terms of the BSD style License as found in the
//~ LICENSE file included with this distribution.
//~
//~ TechGame Networks, LLC can be reached at:
//~ 3578 E. Hartsel Drive #211
//~ Colorado Springs, Colorado, USA, 80920
//~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~ Imports
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <iostream> // for dump
#include "trianglemesh.hpp"

#include <assert.h>

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~ Definitions
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Edge::Edge(int _ev0, int _ev1) : ev0(_ev0), ev1(_ev1)
{
	// ensure it is not degenerate
	if (_ev0 == _ev1)
		assert(false && "Degenerate edge.");
};

bool Edge::operator<(const Edge & otheredge) const
{
	if (ev0 < otheredge.ev0) return true;
	if (ev0 > otheredge.ev0) return false;
	// ev0 == otheredge.ev0, so use ev1 for comparing
	if (ev1 < otheredge.ev1) return true;
	return false;
};

bool Edge::operator==(const Edge & otheredge) const
{
	return ((ev0 == otheredge.ev0) && (ev1 == otheredge.ev1));
};

MEdge::MEdge(const Edge & edge) : Edge(edge), faces()
{
	// nothing to do: faces are set in Mesh::add_face.
};

void MEdge::dump() const
{
#if 0
	std::cout << "  edge " << ev0 << "," << ev1 << std::endl;
	BOOST_FOREACH(std::weak_ptr<MFace> face, faces) {
		if (MFacePtr f = face.lock()) {
			std::cout << "    face " << f->v0 << "," << f->v1 << "," << f->v2 << std::endl;
		};
	};
#endif
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Face::Face(int _v0, int _v1, int _v2)
{
	// ensure it is not degenerate
	if ((_v0 == _v1) || (_v1 == _v2) || (_v0 == _v2))
		assert(false && "Degenerate face.");
	// order vertex indices
	if ((_v0 < _v1) && (_v0 < _v2)) {
		v0 = _v0;
		v1 = _v1;
		v2 = _v2;
	} else if ((_v1 < _v0) && (_v1 < _v2)) {
		v0 = _v1;
		v1 = _v2;
		v2 = _v0;
	} else if ((_v2 < _v0) && (_v2 < _v1)) {
		v0 = _v2;
		v1 = _v0;
		v2 = _v1;
	} else {
		assert(false && "Oops. Face construction bug!");
	}
};

bool Face::operator<(const Face & otherface) const
{
	if (v0 < otherface.v0) return true;
	if (v0 > otherface.v0) return false;
	if (v1 < otherface.v1) return true;
	if (v1 > otherface.v1) return false;
	if (v2 < otherface.v2) return true;
	return false;
};

bool Face::operator==(const Face & otherface) const
{
	return ((v0 == otherface.v0) && (v1 == otherface.v1) && (v2 == otherface.v2));
};

int Face::get_next_vertex(int vi) const
{
	if (vi == v0) return v1;
	else if (vi == v1) return v2;
	else if (vi == v2) return v0;
	// bug!
	assert(false && "Invalid vertex index.");
}

MFace::MFace(const Face & face)
	: Face(face), faces0(), faces1(), faces2(),
	  strip_id(-1), test_strip_id(-1), experiment_id(-1)
{
	// nothing to do
};

MFace::Faces & MFace::get_adjacent_faces(int vi)
{
	if (vi == v0) return faces0;
	else if (vi == v1) return faces1;
	else if (vi == v2) return faces2;
	// bug!
	else { assert(false && "Invalid vertex index."); }
};

void MFace::dump() const
{
#if 0
	std::cout << "  face " << v0 << "," << v1 << "," << v2 << std::endl;
	BOOST_FOREACH(std::weak_ptr<MFace> face0, faces0) {
		if (MFacePtr f = face0.lock()) {
			std::cout << "    face0 " << f->v0 << "," << f->v1 << "," << f->v2 << std::endl;
		};
	}
	BOOST_FOREACH(std::weak_ptr<MFace> face1, faces1) {
		if (MFacePtr f = face1.lock()) {
			std::cout << "    face1 " << f->v0 << "," << f->v1 << "," << f->v2 << std::endl;
		};
	}
	BOOST_FOREACH(std::weak_ptr<MFace> face2, faces2) {
		if (MFacePtr f = face2.lock()) {
			std::cout << "    face2 " << f->v0 << "," << f->v1 << "," << f->v2 << std::endl;
		};
	}
#endif
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void Mesh::add_edge(MFacePtr face, int pv0, int pv1)
{
	// add edge to edge map

	// create edge index and search for edge in mesh
	Edge edge01_index(pv0, pv1);
	EdgeMap::const_iterator edge01_iter = _edges.find(edge01_index);
	if (edge01_iter != _edges.end()) {
		// edge already exists!
		// update list of faces that have this edge
		edge01_iter->second->faces.push_back(face);
	} else {
		// create edge
		MEdgePtr edge01(new MEdge(edge01_index));
		_edges[edge01_index] = edge01;
		// update list of faces that have this edge
		edge01->faces.push_back(face);
	};

	// find adjacent faces

	//    pv0---other_pv2
	//   /  \  /
	// pv2---pv1

	// with other_pv0 == pv1
	// with other_pv1 == pv0

	// create reverse edge index and search for reverse edge in mesh
	Edge edge10_index(pv1, pv0);
	EdgeMap::const_iterator edge10_iter = _edges.find(edge10_index);
	if (edge10_iter != _edges.end()) {
		// the faces of the reverse edge are adjacent faces
		MEdgePtr edge10(edge10_iter->second);
		// find opposite vertex of face
		int pv2 = face->get_next_vertex(pv1);
        for (std::weak_ptr<MFace> _otherface : edge10->faces) {
			if (MFacePtr otherface = _otherface.lock()) {
				int other_pv2 = otherface->get_next_vertex(pv0);
				face->get_adjacent_faces(pv2).push_back(otherface);
				otherface->get_adjacent_faces(other_pv2).push_back(face);
			};
		};
	};
}

Mesh::Mesh() : _faces(), _edges(), faces() {};

MFacePtr Mesh::add_face(int v0, int v1, int v2)
{
	// create face index and search if face already exists in mesh
	Face face_index(v0, v1, v2);
	FaceMap::const_iterator face_iter = _faces.find(face_index);
	if (face_iter != _faces.end()) {
		// face already exists!
		if (MFacePtr old_face = face_iter->second.lock()) {
			return old_face;
		}
	}
	// create face
	MFacePtr face(new MFace(face_index));
	_faces[face_index] = face;
	faces.push_back(face);
	// create edges and update links between faces
	add_edge(face, v0, v1);
	add_edge(face, v1, v2);
	add_edge(face, v2, v0);
	return face;
}

void Mesh::lock()
{
	_edges.clear();
	_faces.clear();
}

void Mesh::dump() const
{
#if 0
	std::cout << faces.size() << " faces" << std::endl;
	BOOST_FOREACH(MFacePtr face, faces) {
		face->dump();
	};
	std::cout << _edges.size() << " edges" << std::endl;
	BOOST_FOREACH(EdgeMap::value_type edge, _edges) {
		edge.second->dump();
	};
#endif
}
