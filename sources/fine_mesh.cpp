#include "fine_mesh.h"
#include "auxiliary_functions.h"
#include <fstream>
#include <map>


FineMesh::FineMesh()
{ }



FineMesh::~FineMesh()
{
  clear();
}



void FineMesh::clear()
{
  _triangles.clear();
  _edges.clear();
}



void FineMesh::read(const std::string &file)
{
  std::ifstream in(file.c_str());
  require(in, "File " + file + " cannot be opened!");

  clear(); // if some mesh has been already read before, we delete it

  std::string str;
  in >> str; // the first string of Gmsh file is "$MeshFormat"
  expect(str == "$MeshFormat",
         "The first string of the Gmsh file " + file +
         " doesn't equal to \"$MeshFormat\". The actual string is \"" + str + "\"");

  // read the information about the mesh
  double version;
  int binary, dsize;
  in >> version >> binary >> dsize;
  expect(version == 2.2,
         "The version of Gmsh's mesh is unexpected (" + d2s(version) + ").");
  expect(dsize == sizeof(double),
         "The size of Gmsh's double (" + d2s(dsize) +\
         ") doesn't equal to size of double type (" + d2s(sizeof(double)) + ")");

  getline(in, str); // read some empty string

  // there is additional 1 (the number - one) in binary format
  if (binary)
  {
    int one;
    in.read(reinterpret_cast<char*>(&one), sizeof(int));
    require(one == 1,
            "The binary one (" + d2s(one) + ") doesn't equal to 1!");
  }

  // we make a map between serial number of the vertex and its number in the file.
  // it will help us when we create mesh elements
  std::map<unsigned int, unsigned int> vertices_map;

  // read lines of mesh file.
  // if we face specific keyword, we'll treat the section.
  while (in >> str)
  {
    if (str == "$Nodes") // read the mesh vertices
    {
      unsigned int n_vertices; // the number of all mesh vertices (that are saved in the file)
      in >> n_vertices; // read that number
      _vertices.resize(n_vertices); // allocate the memory for mesh vertices
      getline(in, str); // read some empty string

      unsigned int number; // the number of the vertex
      double coord[Point::n_coord]; // Cartesian coordinates of the vertex (Gmsh produces 3D mesh regardless its real dimension)
      double max_coord[Point::n_coord]; // the limits of the computational domain: maximum
      double min_coord[Point::n_coord]; // and minimum

      // read vertices
      for (unsigned int ver = 0; ver < n_vertices; ++ver)
      {
        if (binary) // binary format
        {
          in.read(reinterpret_cast<char*>(&number), sizeof(unsigned int)); // the number of each node
          in.read(reinterpret_cast<char*>(coord), Point::n_coord * sizeof(double)); // node coordinates
        }
        else // ASCII format
        {
          in >> number;
          for (unsigned int i = 0; i < Point::n_coord; ++i)
            in >> coord[i];
          if (ver == 0) // for the first vertex
          {
            for (unsigned int i = 0; i < Point::n_coord; ++i)
              max_coord[i] = min_coord[i] = coord[i]; // initialization of the max and min coordinates
          }
          else // for the other vertices
          {
            for (unsigned int i = 0; i < Point::n_coord; ++i)
            {
              max_coord[i] = std::max(max_coord[i], coord[i]); // searching max and min coordinates
              min_coord[i] = std::min(min_coord[i], coord[i]);
            }
          }

        }
        _vertices[ver] = Point(coord); // save the vertex
        vertices_map[number] = ver; // add the number of vertex to the map
      }
      _max_coord = Point(max_coord); // this point can not be one of the mesh vertices if the domain has curvilinear boundaries
      _min_coord = Point(min_coord); // the same as above is true

      expect(n_vertices == vertices_map.size(),
             "Vertices numbers are not unique: n_vertices = " + d2s(n_vertices) +
             " vertices_map.size() = " + d2s(vertices_map.size()));

    } // read the vertices

    else if (str == "$Elements") // read the mesh elements
    {
      unsigned int n_elements; // the number of mesh elements
      in >> n_elements; // read that number
      getline(in, str); // empty string

      unsigned int number; // the number of the element [1, nElements]
      unsigned int el_type; // the type of the element (1 - line, 2 - triangle, etc)
      unsigned int n_tags; // the number of tags describing the element
      unsigned int phys_domain; // the physical domain where the element takes place
      unsigned int elem_domain; // the elementary domain where the element takes place
      unsigned int n_partitions; // the number of partitions in which the element takes place
      unsigned int partition; // the partition which the element belongs to
      std::vector<unsigned int> ghost_cells; // "ghost cells" are other partitions which this element is connected with

      // the map between the type of the element,
      // and the number of nodes that describe it
      std::map<unsigned int, unsigned int> type_nodes;
      type_nodes[1] = 2; // 2-nodes line
      type_nodes[2] = 3; // 3-nodes triangle
      type_nodes[3] = 4; // 4-nodes quadrangle
      type_nodes[4] = 4; // 4-nodes tetrahedron
      type_nodes[5] = 8; // 8-nodes hexahedron
      type_nodes[15]= 1; // 1-node point

      if (binary) // binary format
      {
        require(false, "Binary 2.0 format of Gmsh mesh files is not supported");
      } // binary format

      else // ASCII format
      {
        for (int el = 0; el < n_elements; ++el)
        {
          in >> number >> el_type >> n_tags;
          std::vector<int> data(n_tags); // allocate the memory for some data
          for (unsigned int i = 0; i < n_tags; ++i) // read this information
            in >> data[i];
          phys_domain = (n_tags > 0) ? data[0] : 0; // physical domain - the most important value
          elem_domain = (n_tags > 1) ? data[1] : 0; // elementary domain
          if (n_tags > 2)
          {
            // the number of partitions where this elements takes place
            n_partitions = data[2];
            expect(n_partitions >= 1,
                   "The number of tags is more than 2. That means that we have partitions. But the number of partitions is "
                   + d2s(n_partitions));
            // the partition which the element belongs to
            partition = data[3];
            // "ghost cells"
            if (n_partitions > 1) // if the element is on the boundary between the partitions, it is described by "ghost cells" as well
            {
              ghost_cells.resize(n_partitions - 1);
              for (int gc = 0; gc < n_partitions - 1; ++gc)
                ghost_cells[gc] = -data[4 + gc]; // 'minus' since ghost cells are described by number of partition with the negative sing
            }
          }

          data.clear(); // clear the memory

          // how many vertices (nodes) describe the element
          std::map<unsigned int, unsigned int>::const_iterator el_type_iter =
              type_nodes.find(el_type);

          require(el_type_iter != type_nodes.end(),
                  "This type of the Gmsh's element (" + d2s(el_type) +
                  ") in the mesh file \"" + file + "\" is unknown!");

          const unsigned int n_elem_nodes = el_type_iter->second; // the number of nodes
          std::vector<unsigned int> nodes(n_elem_nodes); // allocate memory for nodes
          for (unsigned int i = 0; i < n_elem_nodes; ++i)
          {
            in >> nodes[i]; // read the numbers of nodes
            // vertices can be numerated not sequentially (or at least not from 0)
            nodes[i] = vertices_map.find(nodes[i])->second;
          }

          // add new element in the list
          //MeshElement *new_element;
          switch (el_type)
          {
//          case 15: // 1-node point
//            points.push_back(MeshElement_ptr(new PhysPoint(nodes, phys_domain)));
//            break;
          case 1: // 2-nodes line
            _lines.push_back(Line(nodes, phys_domain));
            //new_element = new Line(nodes, phys_domain);
            break;
          case 2: // 3-nodes triangle
            _triangles.push_back(Triangle(nodes, _vertices, phys_domain, partition, ghost_cells));
            //new_element = new Triangle(nodes, phys_domain);
            break;
//          case 3: // 4-nodes quadrangle
//            quadrangles.push_back(MeshElement_ptr(new Quadrangle(nodes, phys_domain)));
//            //new_element = new Quadrangle(nodes, phys_domain);
//            break;
//          case 4: //4-nodes tetrahedron
//            tetrahedra.push_back(MeshElement_ptr(new Tetrahedron(nodes, phys_domain)));
//            //new_element = new Tetrahedron(nodes, phys_domain);
//            break;
//          case 5: // 8-nodes hexahedron
//            hexahedra.push_back(MeshElement_ptr(new Hexahedron(nodes, phys_domain)));
//            //new_element = new Hexahedron(nodes, phys_domain);
//            break;
          default:
            require(false,
                    "Unknown type of the Gmsh's element (" + d2s(el_type) +
                    ") in the file " + file + "!");
          }

          nodes.clear();

          //elements.push_back(new_element);
          //delete new_element;
        }

        // check some expectations
        expect(number == n_elements,
               "The number of the last read Gmsh's element (" + d2s(number) +\
               ") is not equal to the amount of all elements in the mesh (" + d2s(n_elements) + ")!");

      } // ASCII format

      // requirements after reading elements
      require(!_triangles.empty(),
             "There are no any 2D or 3D elements in the mesh!");
      //require(!_lines.empty(),
      //       "There are no boundary lines in the mesh!");

    } // read the elements
  } // read the mesh file

  in.close(); // close the file
}



unsigned int FineMesh::n_vertices() const
{
  return _vertices.size();
}



Point FineMesh::vertex(unsigned int number) const
{
  expect(number >= 0 && number < _vertices.size(), "Incorrect input parameter");
  return _vertices[number];
}



const std::vector<Point>& FineMesh::vertices() const
{
  return _vertices;
}



unsigned int FineMesh::n_triangles() const
{
  return _triangles.size();
}



Triangle FineMesh::triangle(unsigned int number) const
{
  expect(number >= 0 && number < _triangles.size(), "Incorrect input parameter");
  return _triangles[number];
}



Triangle* FineMesh::triangle_orig(unsigned int number)
{
  expect(number >= 0 && number < _triangles.size(), "Incorrect input parameter");
  return &_triangles[number];
}



Point FineMesh::max_coord() const
{
  return _max_coord;
}



Point FineMesh::min_coord() const
{
  return _min_coord;
}
