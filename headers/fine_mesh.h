#ifndef FINE_MESH_H
#define FINE_MESH_H

#include "point.h"
#include "triangle.h"
#include "line.h"
#include <vector>
#include <string>


class FineMesh
{
public:
            /**
             * Constructor
             */
  FineMesh();

            /**
             * Destructor
             */
  ~FineMesh();

            /**
             * Read the mesh from a file
             * @param filename - a name of the file with the mesh
             */
  void read(const std::string &filename);

            /**
             * Get the number of mesh vertices
             */
  unsigned int n_vertices() const;

            /**
             * Get the mesh vertex (its copy)
             * @param number - the serial number of the mesh vertex
             */
  Point vertex(unsigned int number) const;

            /**
             * Get a constant reference to all mesh vertices
             */
  const std::vector<Point>& vertices() const;

            /**
             * Get the number of mesh triangles
             */
  unsigned int n_triangles() const;

            /**
             * Get the mesh triangle (its copy)
             * @param number - the serial number of the mesh triangle
             */
  Triangle triangle(unsigned int number) const;

            /**
             * Get the mesh triangle - not a copy - reference to original one
             * @param number - the serial number of the mesh triangle
             */
  Triangle* triangle_orig(unsigned int number);

            /**
             * Get the maximal coordinates of the domain
             */
  Point max_coord() const;

            /**
             * Get the minimal coordinates of the domain
             */
  Point min_coord() const;


private:
            /**
             * Mesh vertices
             */
  std::vector<Point> _vertices;

            /**
             * Maximal coordinates of the mesh vertices.
             * max_coord can not be one of the mesh vertices, if
             * the computational domain has curvilinear boundaries
             */
  Point _max_coord;

            /**
             * Minimal coordinates of the mesh vertices.
             * min_coord can not be one of the mesh vertices, if
             * the computational domain has curvilinear boundaries
             */
  Point _min_coord;

            /**
             * Mesh triangles
             */
  std::vector<Triangle> _triangles;

            /**
             * Mesh lines (like boundary lines)
             */
  std::vector<Line> _lines;

            /**
             * Mesh edges
             */
  std::vector<Line> _edges;

  FineMesh(const FineMesh&);
  FineMesh& operator=(const FineMesh&);

            /**
             * Free the memory
             */
  void clear();
};

#endif // FINE_MESH_H
