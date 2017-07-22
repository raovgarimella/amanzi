/*
  WhetStone, version 2.1
  Release name: naka-to.

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Author: Konstantin Lipnikov (lipnikov@lanl.gov)

  Discontinuous Galerkin methods.
*/

#include "Point.hh"

#include "DenseMatrix.hh"
#include "DG_Modal.hh"
#include "Polynomial.hh"
#include "WhetStoneDefs.hh"

namespace Amanzi {
namespace WhetStone {

/* ******************************************************************
* Mass matrix for Taylor basis functions. 
****************************************************************** */
int DG_Modal::MassMatrix(int c, const Tensor& K, DenseMatrix& M)
{
  // calculate all monomials
  Polynomial integrals(d_, 2 * order_);

  integrals.monomials(0).coefs()[0] = mesh_->cell_volume(c);

  for (int k = 1; k <= integrals.order(); ++k) {
    IntegrateMonomialsCell_(c, integrals.monomials(k));
  }
   
  // copy integrals to mass matrix
  int multi_index[3];
  Polynomial p(d_, order_), q(d_, order_);

  int nrows = p.size();
  M.Reshape(nrows, nrows);

  for (auto it = p.begin(); it.end() <= p.end(); ++it) {
    const int* idx_p = it.multi_index();
    int k = p.PolynomialPosition(idx_p);

    for (auto jt = q.begin(); jt.end() <= q.end(); ++jt) {
      const int* idx_q = jt.multi_index();
      int l = q.PolynomialPosition(idx_q);
      
      int n(0);
      for (int i = 0; i < d_; ++i) {
        multi_index[i] = idx_p[i] + idx_q[i];
        n += multi_index[i];
      }

      const auto& coefs = integrals.monomials(n).coefs();
      M(k, l) = K(0, 0) * coefs[p.MonomialPosition(multi_index)];
    }
  }

  return 0;
}


/* ******************************************************************
* Advection matrix for Taylor basis and polynomial velocity u.
****************************************************************** */
int DG_Modal::AdvectionMatrixCell(int c, Polynomial& divu, DenseMatrix& A)
{
  // calculate monomials
  int uk(divu.order());
  Polynomial integrals(d_, 2 * order_ + uk);

  integrals.monomials(0).coefs()[0] = mesh_->cell_volume(c);

  for (int k = 1; k <= integrals.order(); ++k) {
    IntegrateMonomialsCell_(c, integrals.monomials(k));
  }

  // copy integrals to mass matrix
  int multi_index[3];
  Polynomial p(d_, order_), q(d_, order_);

  int nrows = p.size();
  A.Reshape(nrows, nrows);
  A.PutScalar(0.0);

  for (auto it = p.begin(); it.end() <= p.end(); ++it) {
    const int* idx_p = it.multi_index();
    int k = p.PolynomialPosition(idx_p);

    for (auto mt = divu.begin(); mt.end() <= divu.end(); ++mt) {
      const int* idx_divu = mt.multi_index();
      int m = divu.MonomialPosition(idx_divu);
      double factor = divu.monomials(mt.end()).coefs()[m];

      for (auto jt = q.begin(); jt.end() <= q.end(); ++jt) {
        const int* idx_q = jt.multi_index();
        int l = q.PolynomialPosition(idx_q);

        int n(0);
        for (int i = 0; i < d_; ++i) {
          multi_index[i] = idx_p[i] + idx_q[i] + idx_divu[i];
          n += multi_index[i];
        }

        const auto& coefs = integrals.monomials(n).coefs();
        A(k, l) -= factor * coefs[p.MonomialPosition(multi_index)];
      }
    }
  }

  return 0;
}


/* ******************************************************************
* Advection matrix for Taylor basis and normal velocity u.n.
* Velocity is given in the face-based Taylor basis.
****************************************************************** */
int DG_Modal::AdvectionMatrixFace(int f, Polynomial& un, DenseMatrix& A)
{
  AmanziMesh::Entity_ID_List cells, nodes;

  mesh_->face_get_cells(f, AmanziMesh::USED, &cells);
  int ncells = cells.size();

  A.Reshape(ncells, ncells);  // hack
  A.PutScalar(0.0);

  // identify downwind cell
  int dir, id(0); 
  double vel = un.monomials(0).coefs()[0];
  mesh_->face_normal(f, false, cells[0], &dir);
  if (vel * dir > 0.0) {
    if (ncells == 1) return 0;
    id = 1;
  }

  // integrate traces from downwind cell
  double umod = fabs(vel);
  if (ncells == 1) {
    A(0, 0) = -umod;
  } else {
    A(id, id) = -umod;
    A(1 - id, id) = umod;
  }
}


/* ******************************************************************
* Integrate all specified monomials in cell c.
* The cell must be star-shape w.r.t. to its centroid.
****************************************************************** */
void DG_Modal::IntegrateMonomialsCell_(int c, Monomial& monomials)
{
  int k = monomials.order();

  Entity_ID_List faces, nodes;
  std::vector<int> dirs;

  mesh_->cell_get_faces_and_dirs(c, &faces, &dirs);
  int nfaces = faces.size();

  const AmanziGeometry::Point& xc = mesh_->cell_centroid(c);

  for (int n = 0; n < nfaces; ++n) {
    int f = faces[n];
    const AmanziGeometry::Point& xf = mesh_->face_centroid(f);
    const AmanziGeometry::Point& normal = mesh_->face_normal(f);
    double tmp = dirs[n] * ((xf - xc) * normal) / (k + d_);
    
    if (d_ == 3) {
      IntegrateMonomialsFace_(f, tmp, monomials);
    } else if (d_ == 2) {
      mesh_->face_get_nodes(f, &nodes);

      AmanziGeometry::Point x1(d_), x2(d_);
      mesh_->node_get_coordinates(nodes[0], &x1);
      mesh_->node_get_coordinates(nodes[1], &x2);

      x1 -= xc;
      x2 -= xc;
      IntegrateMonomialsEdge_(x1, x2, tmp, monomials);
    }
  }
}


/* ******************************************************************
* Integrate all monomials of order k on face.
****************************************************************** */
void DG_Modal::IntegrateMonomialsFace_(int c, double factor, Monomial& monomials)
{
}


/* ******************************************************************
* Integrate all monomials of order k on edge via quadrature rules.
****************************************************************** */
void DG_Modal::IntegrateMonomialsEdge_(
    const AmanziGeometry::Point& x1, const AmanziGeometry::Point& x2,
    double factor, Monomial& monomials)
{
  int k = monomials.order();
  AmanziGeometry::Point xm(d_);
  auto& coefs = monomials.coefs();

  if (d_ == 2) {
    int m = k / 2;  // calculate quadrature rule

    for (int i = 0; i <= k; ++i) {
      for (int n = 0; n <= m; ++n) { 
        xm = x1 * q1d_points[m][n] + x2 * (1.0 - q1d_points[m][n]);
        double a1 = std::pow(xm[0], k - i) * std::pow(xm[1], i);
        coefs[i] += factor * a1 * q1d_weights[m][n];      
      }
    }
  }
}


/* ******************************************************************
* Integrate two monomials of order k on edge via quadrature rules.
****************************************************************** */
double DG_Modal::IntegrateMonomialsEdge_(
    const AmanziGeometry::Point& x1, const AmanziGeometry::Point& x2,
    int ix, int iy, int jx, int jy, 
    const std::vector<double>& factors, 
    const AmanziGeometry::Point& xc1, const AmanziGeometry::Point& xc2)
{
  double a1, a2, a3, tmp(0.0); 
  AmanziGeometry::Point xm(d_), ym(d_), xe((x1 + x2) /2);

  if (d_ == 2) {
    int uk(std::pow(2 * factors.size(), 0.5) - 1);
    int m((ix + iy + jx + jy + uk) / 2);  // calculate quadrature rule

    for (int n = 0; n <= m; ++n) { 
      xm = x1 * q1d_points[m][n] + x2 * (1.0 - q1d_points[m][n]);

      a3 = factors[0];
      if (uk > 0) {  // FIXME
        ym = xm - xe;  
        a3 += factors[1] * ym[0] + factors[2] * ym[1];
      }      

      ym = xm - xc1;
      a1 = std::pow(ym[0], ix) * std::pow(ym[1], iy);

      xm -= xc2;
      a2 = std::pow(xm[0], jx) * std::pow(xm[1], jy);

      tmp += a1 * a2 * a3 * q1d_weights[m][n];      
    }
  }

  return tmp;
}


/* ******************************************************************
* Polynomial approximation of map x2 = F(x1).
* We assume that vectors of vertices have a proper length.
****************************************************************** */
int DG_Modal::LeastSquareFit(const std::vector<AmanziGeometry::Point>& x1, 
                             const std::vector<AmanziGeometry::Point>& x2,
                             std::vector<AmanziGeometry::Point>& u) const
{
  int nk = (order_ + 1) * (order_ + 2) / 2;
  int nx = x1.size();

  // evaluate basis functions at given points
  int i1(0);
  DenseMatrix psi(nk, nx);

  for (int k = 0; k <= order_; ++k) {
    for (int i = 0; i < k + 1; ++i) {
      for (int n = 0; n < nx; ++n) {
        psi(i1, n) = std::pow(x1[n][0], k - i) * std::pow(x1[n][1], i);
      }
      i1++;
    }
  }
      
  // form linear system
  DenseMatrix A(nk, nk);
  DenseVector bx(nk), by(nk), ux(nk), uy(nk);

  for (int i = 0; i < nk; ++i) {
    for (int j = i; j < nk; ++j) {
      double tmp(0.0);
      for (int n = 0; n < nx; ++n) {
        tmp += psi(i, n) * psi(j, n);
      }
      A(i, j) = A(j, i) = tmp;
    }

    bx(i) = 0.0;
    by(i) = 0.0;
    for (int n = 0; n < nx; ++n) {
      bx(i) += x2[n][0] * psi(i, n);
      by(i) += x2[n][1] * psi(i, n);
    }
  }

  // solver linear systems
  A.Inverse();
  A.Multiply(bx, ux, false);
  A.Multiply(by, uy, false);

  u.clear();
  for (int i = 0; i < nk; ++i) {
    u.push_back(AmanziGeometry::Point(ux(i), uy(i)));
  }
}

}  // namespace WhetStone
}  // namespace Amanzi

