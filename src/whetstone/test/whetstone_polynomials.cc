/*
  WhetStone

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Author: Konstantin Lipnikov (lipnikov@lanl.gov)
*/

#include <cstdlib>
#include <cmath>
#include <iostream>

// TPLs
#include "UnitTest++.h"

// Amanzi::WhetStone
#include "Polynomial.hh"
#include "VectorPolynomial.hh"


/* ****************************************************************
* Test of Taylor polynomials
**************************************************************** */
TEST(DG_TAYLOR_POLYNOMIALS) {
  using namespace Amanzi;
  using namespace Amanzi::WhetStone;

  // polynomials in two dimentions
  Polynomial p(2, 3);

  int i(0);
  for (auto it = p.begin(); it < p.end(); ++it) {
    const int* index = it.multi_index();
    CHECK(index[0] >= 0 && index[1] >= 0);

    int pos = PolynomialPosition(2, index);
    CHECK(pos == i++);

    int m = MonomialSetPosition(2, index);
    p(index[0] + index[1], m) = pos;
  }
  std::cout << p << std::endl; 
  CHECK(p.size() == 10);

  // re-define polynomials
  p.Reshape(2, 4);
  std::cout << p << std::endl; 
  CHECK(p.size() == 15);

  Polynomial p_tmp(p);
  p.Reshape(2, 2);
  std::cout << "Reshaping last polynomial\n" << p << std::endl; 
  CHECK(p.size() == 6);

  // operations with polynomials
  AmanziGeometry::Point xy(1.0, 0.0);
  double val = p.Value(xy) + p_tmp.Value(xy);

  p += p_tmp;
  CHECK(p.size() == 15);
  CHECK_CLOSE(p.Value(xy), val, 1e-12);

  // polynomials in 3D
  Polynomial q(3, 3);

  i = 0;
  for (auto it = q.begin(); it < q.end(); ++it) {
    const int* index = it.multi_index();
    CHECK(index[0] >= 0 && index[1] >= 0 && index[2] >= 0);

    int pos = PolynomialPosition(3, index);
    CHECK(pos == i++);

    int m = MonomialSetPosition(3, index);
    q(index[0] + index[1] + index[2], m) = pos;
  }
  std::cout << "Original polynomial\n" << q << std::endl; 
  CHECK(q.size() == 20);
  Polynomial q_orig(q);

  // reshape polynomials
  q.Reshape(3, 2);
  Polynomial q1(q), q2(q), q3(q);
  std::cout << "Reshaping last 3D polynomial\n" << q << std::endl; 
  CHECK(q.size() == 10);

  q.Reshape(3, 3);
  std::cout << "Reshaping last 3D polynomial, (name q)\n" << q << std::endl; 
  CHECK(q.size() == 20);

  // ring operations with polynomials
  AmanziGeometry::Point xyz(1.0, 2.0, 3.0);
  val = q1.Value(xyz);
  q1 *= q2;
  CHECK_CLOSE(q1.Value(xyz), val * val, 1e-10);

  val = q2.Value(xyz);
  q2 *= q2;
  CHECK_CLOSE(q2.Value(xyz), val * val, 1e-10);

  Polynomial q4 = q2 - q3 * q3;
  CHECK_CLOSE(q4.Value(xyz), 0.0, 1e-10);

  // derivatives
  auto grad = Gradient(q_orig);
  std::cout << "Gradient of a polynomial:\n" << grad << std::endl;
 
  Polynomial lp = q_orig.Laplacian();
  std::cout << "Laplacian of original polynomial:\n" << lp << std::endl;

  q4 = Divergence(grad) - lp; 
  CHECK_CLOSE(0.0, q4.NormInf(), 1e-12);

  // change origin of coordinate system
  AmanziGeometry::Point origin(0.5, 0.3, 0.2);
  q.Reshape(3, 2);
  val = q.Value(xyz);
  q.ChangeOrigin(origin);
  std::cout << "Changed origin of polynomial q\n" << q << std::endl; 
  CHECK_CLOSE(val, q.Value(xyz), 1e-10);

  // trace of a 2D polynomial
  Polynomial p2d(2, 3);

  for (auto it = p2d.begin(); it < p2d.end(); ++it) {
    const int* index = it.multi_index();
    int pos = PolynomialPosition(2, index);

    int m = it.MonomialSetOrder();
    int k = it.MonomialSetPosition();
    p2d(m, k) = pos;
  }
  AmanziGeometry::Point x0(0.0, 0.0), v1(1.0, 1.0);
  std::vector<AmanziGeometry::Point> tau;
  tau.push_back(v1);

  Polynomial p1d(p2d);
  p1d.ChangeCoordinates(x0, tau);
  std::cout << "tau[0]=" << tau[0] << std::endl;
  std::cout << "Before ChangeCoordinates: " << p2d << std::endl;
  std::cout << "After ChangeCoordinates: " << p1d << std::endl;

  p2d.set_origin(AmanziGeometry::Point(1.0, 1.0));
  p1d = p2d;
  p1d.ChangeCoordinates(x0, tau);
  std::cout << "Before ChangeCoordinates: " << p2d << std::endl;
  std::cout << "After ChangeCoordinates: " << p1d << std::endl;

  // assignement small to large
  q1.Reshape(2, 3, true);
  q2.Reshape(2, 2, true);
  q1 = q2;
}
