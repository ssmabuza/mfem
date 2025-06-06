// Copyright (c) 2010-2025, Lawrence Livermore National Security, LLC. Produced
// at the Lawrence Livermore National Laboratory. All Rights reserved. See files
// LICENSE and NOTICE for details. LLNL-CODE-806117.
//
// This file is part of the MFEM library. For more information and source code
// availability visit https://mfem.org.
//
// MFEM is free software; you can redistribute it and/or modify it under the
// terms of the BSD-3 license. We welcome feedback and contributions, see file
// CONTRIBUTING.md for details.
//
//             -----------------------------------------------
//             Spiral Miniapp:  Animation of a spiral cone toy
//             -----------------------------------------------
//
// Model of an interesting fidget spiral cone toy. The toy is made out of two
// parts which can (surprisingly) pass through each other regardless of their
// orientation.
//
// - STL model by Per Lundberg from https://www.thingiverse.com/thing:6682243
// - Surface mesh generated by Gmsh (NOTE: not suitable for FEM simulations)
//
// NOTE: This miniapp requires a large mesh that is stored in the mfem/data
//       repository. Make sure to clone it from https://github.com/mfem/data.
//
// Compile with: make spiral
//
// Sample runs:  spiral
//               spiral -c 2 -s 200
//               spiral -c 3 -no-col
//               spiral -c 7 -s 400 --movie
//               spiral -c 8

#include "mfem.hpp"
#include <fstream>
#include <iostream>

using namespace std;
using namespace mfem;

int main(int argc, char *argv[])
{
   // Parse command-line options.
   string mfem_data_dir = "../../../data";
   int conf = 1;
   int steps = 100;
   bool color = true;
   bool movie = false;
   bool visualization = true;

   OptionsParser args(argc, argv);
   args.AddOption(&mfem_data_dir, "-data", "--mfem_data_dir",
                  "Path to the mfem/data repo (required). Clone it from:\n"
                  "\t\thttps://github.com/mfem/data");
   args.AddOption(&conf, "-c", "--configuration",
                  "Which configuration of the two parts to animate.\n"
                  "\tThere are 8 options denoted as P1 -> P2 for part P1\n"
                  "\tpassing through part P2, using the following notation:\n"
                  "\tI/O = inner/outer part, S/F = sharp/flat end.\n"
                  "\t\t1) OS -> IS  5) IS -> OS \n"
                  "\t\t2) OF -> IS  6) IS -> OF \n"
                  "\t\t3) OS -> IF  7) IF -> OS \n"
                  "\t\t4) OF -> IF  8) IF -> OF ");
   args.AddOption(&steps, "-s", "--steps",
                  "Number of visualization steps in the animation.");
   args.AddOption(&color, "-col", "--color", "-no-col", "--no-color",
                  "Visualize the parts with different colors.");
   args.AddOption(&movie, "-mov", "--movie", "-no-mov", "--no-movie",
                  "Ask GLVis to take screenshots to make a movie.");
   args.AddOption(&visualization, "-vis", "--visualization", "-no-vis",
                  "--no-visualization",
                  "Enable or disable GLVis visualization.");
   args.Parse();
   if (!args.Good())
   {
      args.PrintUsage(cout);
      return 1;
   }
   args.PrintOptions(cout);

   // Check that the required mesh (spiral-toy.vtk from mfem/data repo) exists
   string mesh_file = mfem_data_dir + "/vtk/spiral-toy.vtk";
   ifstream file(mesh_file);
   if (!file.good())
   {
      cout << "Can't find the mesh file '" << mesh_file << "'\n"
           << "Make sure the github.com/mfem/data repository is cloned in '"
           << mfem_data_dir << "'\n";
      return 2;
   }

   // Check for valid configuration
   if (conf < 1 || conf > 8)
   {
      cerr << "Configuration option should be between 1 and 8. Setting to 1.\n";
      conf = 1;
   }

   // Load mesh with the two spiral parts (below "inner" and "outer")
   Mesh mesh(mfem_data_dir+"/vtk/spiral-toy.vtk", 1, 1);
   H1_FECollection fec(1, 3);
   FiniteElementSpace fespace(&mesh, &fec);
   GridFunction part(&fespace);

   // Mark inner/outer part vertices with 1.0/2.0 in the grid function part
   for (int i = 0; i < mesh.GetNV(); i++)
   {
      real_t *v = mesh.GetVertex(i);
      part[i] = (v[1] < -26.55) ? 1.0 : 2.0;
   }

   // Set different attributes for elements in inner/outer parts
   for (int i = 0; i < mesh.GetNE(); i++)
   {
      Vector c(3);
      mesh.GetElementCenter(i, c);
      mesh.SetAttribute(i, (c(1) < -26.55) ? 1 : 2);
   }

   // Initiate visualization
   char vishost[] = "localhost";
   int  visport   = 19916;
   socketstream sol_sock;

   // Animate the two parts passing each other
   for (int d = 0; d <= steps; d++)
   {
      // Fraction of the animation, from 0 to 1
      real_t frac = ((real_t) d) / steps;

      // Angle of rotation as function of frac
      real_t phi = 0.0;
      if (conf == 1 || conf == 5)
      {
         phi = -(10 * frac + 3.7);
      }
      if (conf == 2 || conf == 6)
      {
         phi = -(10 * frac + 2.8);
      }
      if (conf == 3 || conf == 7)
      {
         phi = -(10 * frac + 2.8);
      }
      if (conf == 4 || conf == 8)
      {
         phi = -(10 * frac + 3.2);
      }

      // Copy the mesh, as we will move the parts below
      Mesh mesh2(mesh);

      for (int i = 0; i < mesh.GetNV(); i++)
      {
         real_t *v = mesh2.GetVertex(i);

         // Temporary variables
         real_t v0 = 0.0;
         real_t v1 = 0.0;
         real_t v2 = 0.0;

         // Move and rotate the outer part
         if (part[i] == 1.0)
         {
            if (conf == 1 || conf == 3 || conf == 5 || conf == 7)
            {
               v0 = -v[0];
               v1 = v[1] + 50.45;
               v2 = 133.4 - 133.4*frac - v[2];
            }

            if (conf == 2 || conf == 4 || conf == 6 || conf == 8)
            {
               v0 = v[0];
               v1 = v[1] + 50.45;
               v2 = 73.04 - 133.4*frac + v[2];
            }

            if (conf == 1 || conf == 2 || conf == 3 || conf == 4)
            {
               v[0] = cos(phi)*v0 - sin(phi)*v1;
               v[1] = sin(phi)*v0 + cos(phi)*v1;
               v[2] = v2;
            }

            if (conf == 5 || conf == 6 || conf == 7 || conf == 8)
            {
               v[0] = -v0;
               v[1] = v1;
               v[2] = 133.4-v2;
            }
         }

         // Move and rotate the inner part
         if (part[i] == 2.0)
         {
            if  (conf == 3 || conf == 4)
            {
               v[0] = -v[0];
               v[2] = 73.04 - v[2];
            }

            if  (conf == 5 || conf == 6)
            {
               v0 = -v[0];
               v1 = v[1];
               v[0] = cos(phi)*v0 - sin(phi)*v1;
               v[1] = sin(phi)*v0 + cos(phi)*v1;
               v[2] = 133.4-v[2];
            }

            if  (conf == 7 || conf == 8)
            {
               v0 = v[0];
               v1 = v[1];
               v[0] = cos(phi)*v0 - sin(phi)*v1;
               v[1] = sin(phi)*v0 + cos(phi)*v1;
               v[2] = 60.36+v[2];
            }
         }
      }

      // Visualize the current configuration
      if (visualization)
      {
         if (d == 0) // initial setup
         {
            sol_sock.open(vishost, visport);
            sol_sock.precision(8);

            if (color)
            {
               sol_sock << "solution\n" << mesh2 << part
                        << "keys A\n" << "palette 16\n";
            }
            else
            {
               sol_sock << "mesh\n" << mesh2
                        << "keys maaappppppptA\n" << "palette 12\n";
            }

            sol_sock << "window_geometry 0 0 500 1000\n"
                     << "zoom 3\n" << "autoscale on\n" << "pause\n";

            cout << "To see the animation, press 'space' in the GLVis window\n";
         }
         else
         {
            if (color) // after the first frame
            {
               sol_sock << "solution\n" << mesh2 << part;
            }
            else
            {
               sol_sock << "mesh\n" << mesh2;
            }
         }

         if (movie)
         {
            sol_sock << "screenshot spiral"
                     << setfill('0') << setw(3) << d << ".png\n";
         }

         sol_sock << flush;
      }
   }

   if (movie)
   {
      cout << "A sequence of screenshot files: spiral000.png ... spiral"
           << setfill('0') << setw(3) << steps << ".png\n"
           << "have been saved in the directory from which GLVis is running.\n";
   }

   return 0;
}
