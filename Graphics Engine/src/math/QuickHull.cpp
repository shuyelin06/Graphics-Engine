#include "QuickHull.h"

#include <assert.h>
#include <unordered_map>

#include "Plane.h"

namespace Engine {
namespace Math {
// An implementation of the QuickHull algorithm for 3D Convex Hull generation.
// Callable through the QuickHullSolver class.
// Epsilon is set quite large here. Larger values give us a lower chance of
// imprecision error, but also can yield less accurate hulls
static constexpr float EPSILON = 3.5f;

static constexpr int UNASSIGNED = -1;
static constexpr int INSIDE = -2;

struct QuickHullPoint {
    Vector3 position;

    // Points are assigned to a face that they're "above".
    int face;

    QuickHullPoint(const Vector3& pos) {
        position = pos;
        face = UNASSIGNED;
    }
};

struct QuickHullFace {
    // Indices of points that comprise the face
    int i_points[3];

    // Indices of faces which border the same edge opposite of point _
    int i_opposite_faces[3];

    // Marks whether or not the face is in the convex hull or not
    bool in_convex_hull;

    // Marks (during the horizon edge search) if the face has been traversed or
    // not
    bool traversal_flag;

    QuickHullFace(int v0, int v1, int v2, int f0, int f1, int f2)
        : i_points{v0, v1, v2}, i_opposite_faces{f0, f1, f2} {
        in_convex_hull = true;
        traversal_flag = false;
    }
};

struct HorizonEdge {
    int point_1;
    int point_2;

    int visible_face;
    int nonvisible_face;

    HorizonEdge(int p1, int p2, int f1, int f2) {
        point_1 = p1;
        point_2 = p2;
        visible_face = f1;
        nonvisible_face = f2;
    }
};

struct QuickHullData {
    std::vector<QuickHullPoint> points;
    std::vector<QuickHullFace> faces;
    std::vector<HorizonEdge> horizon_edge;

    QuickHullData() : points(), faces(), horizon_edge() {}

    // Given a face and a point, returns the signed distance of the point to the
    // face. If negative, this means that the point is below the face.
    float signedDistanceTo(int i_face, int i_point) const {
        const Vector3& point = points[i_point].position;

        const QuickHullFace& face = faces[i_face];
        const Vector3& v0 = points[face.i_points[0]].position;
        const Vector3& v1 = points[face.i_points[1]].position;
        const Vector3& v2 = points[face.i_points[2]].position;

        const Vector3 normal = (v2 - v0).cross(v1 - v0).unit();

        const float distance = (point - v0).dot(normal);

        return distance;
    }
};

// Constructor / Destructor:
// Create an instance of a QuickHullSolver.
QuickHullSolver::QuickHullSolver() { solver_data = new QuickHullData(); }
QuickHullSolver::~QuickHullSolver() { delete solver_data; }

// GetHull:
// Given the solver_data, creates an instance of a convex hull and returns it.
ConvexHull* QuickHullSolver::getHull() const {
    ConvexHull* hull = new ConvexHull();

    // Translate my data to a triangle strip!
    hull->vertices.clear();
    hull->indices.clear();

    // Maps QuickHull indices -> ConvexHull indices
    std::unordered_map<int, int> point_map;

    for (const QuickHullFace& face : solver_data->faces) {
        if (face.in_convex_hull) {
            // Add points to vertex vector (and save index mapping) if they
            // do not yet exist.
            for (int i = 0; i < 3; i++) {
                const int point_index = face.i_points[i];

                if (!point_map.contains(point_index)) {
                    const int index = hull->vertices.size();
                    hull->vertices.push_back(
                        solver_data->points[point_index].position);
                    point_map[point_index] = index;
                }
            }

            // Add face to index buffer
            const int i0 = point_map[face.i_points[0]];
            const int i1 = point_map[face.i_points[1]];
            const int i2 = point_map[face.i_points[2]];

            hull->indices.push_back(i0);
            hull->indices.push_back(i1);
            hull->indices.push_back(i2);
        }
    }

    return hull;
}

// ClosestFaceToOrigin:
// Given the current hull stored by the QuickHullSolver, finds the face that is closest to the origin. Used
// in the EPA algorithm (see GJK.cpp)
Triangle QuickHullSolver::closestFaceToOrigin(float* distance_out) const {
    Triangle closest_face;
    float distance = FLT_MAX;

    for (const QuickHullFace& face : solver_data->faces) {
        if (face.in_convex_hull) {
            const Vector3& v0 = solver_data->points[face.i_points[0]].position;
            const Vector3& v1 = solver_data->points[face.i_points[1]].position;
            const Vector3& v2 = solver_data->points[face.i_points[2]].position;
            
            const Triangle triangle = Triangle(v0, v1, v2);
            
            const float cur_distance = Plane(triangle.normal(), triangle.center()).distanceTo(Vector3(0,0,0));
            
            if (cur_distance < distance) {
                closest_face = triangle;
                distance = cur_distance;
            }
        }
    }
    
    if (distance_out != nullptr)
        *distance_out = distance;

    return closest_face;
}

// ComputeConvexHull:
// Given a point set, incrementally builds the 3D convex hull for the set
// using the QuickHull algorithm.
void QuickHullSolver::computeConvexHull(
    const std::vector<Vector3>& point_cloud) {
    // Generate my initial hull, and convert all points to QuickHullPoints.
    generateInitialHull(point_cloud);

    // REPEAT:
    // 1) Choose the furthest point from any given face.
    // 2) Find the faces that the point is above.
    // 3) Remove these faces, and create new ones connecting the horizon edge to
    //      the point
    // 4) Update the face indices the points are outside of End when
    //      all points are inside the hull.
    bool stop = false;

    while (!stop) {
        // For every point, assign it to a face that it is outside of.
        // Find the furthest point that is not inside the convex hull.
        int furthest_point = reassignPointsToFaces();

        // If all points are inside the hull, stop.
        if (furthest_point == -1)
            stop = true;
        else {
            // Otherwise, first find the horizon edge from the point.
            solver_data->horizon_edge.clear();
            for (int i = 0; i < solver_data->faces.size(); i++)
                solver_data->faces[i].traversal_flag = false;

            findHorizonEdge(furthest_point,
                            solver_data->points[furthest_point].face, -1);

            // Now, remove all visible faces, and for all horizon edge, generate
            // a new face with that edge and the furthest point.
            const int first_index = solver_data->faces.size();
            const int last_index = solver_data->faces.size() +
                                   solver_data->horizon_edge.size() - 1;

            int prev_index = last_index;
            int next_index = first_index + 1;

            for (const HorizonEdge& edge : solver_data->horizon_edge) {
                // Create my new face
                const int new_face_index = solver_data->faces.size();
                solver_data->faces.push_back(QuickHullFace(
                    edge.point_1, edge.point_2, furthest_point, next_index,
                    prev_index, edge.nonvisible_face));

                // Update face index references
                prev_index = new_face_index;

                if (next_index == last_index)
                    next_index = first_index;
                else
                    next_index = next_index + 1;

                // Remove visible face
                solver_data->faces[edge.visible_face].in_convex_hull = false;

                // Update non-visible face
                QuickHullFace& nonvisible_face =
                    solver_data->faces[edge.nonvisible_face];

                if (nonvisible_face.i_opposite_faces[0] == edge.visible_face)
                    nonvisible_face.i_opposite_faces[0] = new_face_index;
                else if (nonvisible_face.i_opposite_faces[1] ==
                         edge.visible_face)
                    nonvisible_face.i_opposite_faces[1] = new_face_index;
                else if (nonvisible_face.i_opposite_faces[2] ==
                         edge.visible_face)
                    nonvisible_face.i_opposite_faces[2] = new_face_index;
                // This can happen, though I'm not sure exactly why. I'm
                // guessing it can fire due to imprecision (hence why I turned
                // Epsilon up high).
                else
                    assert(false);
            }
        }
    }
}

// AddPointToHull:
// Given a point, adds it to the existing convex hull.
void QuickHullSolver::addPointToHull(const Vector3& point) {
    solver_data->points.push_back(QuickHullPoint(point));

    // If there are not enough points to create a convex hull, do nothing else.
    if (solver_data->points.size() < 4)
        return;

    // REPEAT:
    // 1) Choose the furthest point from any given face.
    // 2) Find the faces that the point is above.
    // 3) Remove these faces, and create new ones connecting the horizon edge to
    //      the point
    // 4) Update the face indices the points are outside of End when
    //      all points are inside the hull.
    bool stop = false;

    while (!stop) {
        // For every point, assign it to a face that it is outside of.
        // Find the furthest point that is not inside the convex hull.
        int furthest_point = reassignPointsToFaces();

        // If all points are inside the hull, stop.
        if (furthest_point == -1)
            stop = true;
        else {
            // Otherwise, first find the horizon edge from the point.
            solver_data->horizon_edge.clear();
            for (int i = 0; i < solver_data->faces.size(); i++)
                solver_data->faces[i].traversal_flag = false;

            findHorizonEdge(furthest_point,
                            solver_data->points[furthest_point].face, -1);

            // Now, remove all visible faces, and for all horizon edge, generate
            // a new face with that edge and the furthest point.
            const int first_index = solver_data->faces.size();
            const int last_index = solver_data->faces.size() +
                                   solver_data->horizon_edge.size() - 1;

            int prev_index = last_index;
            int next_index = first_index + 1;

            for (const HorizonEdge& edge : solver_data->horizon_edge) {
                // Create my new face
                const int new_face_index = solver_data->faces.size();
                solver_data->faces.push_back(QuickHullFace(
                    edge.point_1, edge.point_2, furthest_point, next_index,
                    prev_index, edge.nonvisible_face));

                // Update face index references
                prev_index = new_face_index;

                if (next_index == last_index)
                    next_index = first_index;
                else
                    next_index = next_index + 1;

                // Remove visible face
                solver_data->faces[edge.visible_face].in_convex_hull = false;

                // Update non-visible face
                QuickHullFace& nonvisible_face =
                    solver_data->faces[edge.nonvisible_face];

                if (nonvisible_face.i_opposite_faces[0] == edge.visible_face)
                    nonvisible_face.i_opposite_faces[0] = new_face_index;
                else if (nonvisible_face.i_opposite_faces[1] ==
                         edge.visible_face)
                    nonvisible_face.i_opposite_faces[1] = new_face_index;
                else if (nonvisible_face.i_opposite_faces[2] ==
                         edge.visible_face)
                    nonvisible_face.i_opposite_faces[2] = new_face_index;
                // This can happen, though I'm not sure exactly why. I'm
                // guessing it can fire due to imprecision (hence why I turned
                // Epsilon up high).
                else
                    assert(false);
            }
        }
    }
}

// GenerateInitialHull:
// Generate the starting hull for the convex hull. This is two triangles on top
// of each other, facing in opposite directions.
void QuickHullSolver::generateInitialHull(
    const std::vector<Vector3>& point_cloud) {
    // Generate my initial hull, and convert all points to QuickHullPoints.
    // 1) Select min / max points in x to form a line
    // 2) Find the point furthest from this line to form a triangle
    // 3) Generate two triangles, one facing upwards and one facing downwards.
    int a = -1, b = -1, c = -1, d = -1;
    float furthest_distance;

    // Find points with min / max x, and register them into the points
    // vector
    for (int i = 0; i < point_cloud.size(); i++) {
        const Vector3& position = point_cloud[i];

        const int point_index = solver_data->points.size();
        solver_data->points.push_back(QuickHullPoint(point_cloud[i]));

        if (a == -1 || position.x < solver_data->points[a].position.x)
            a = point_index;
        if (b == -1 || position.x > solver_data->points[b].position.x)
            b = point_index;
    }

    assert(a != b);

    // Find the point furthest from the line formed by a --> b.
    const Vector3& a_pos = solver_data->points[a].position;
    const Vector3& b_pos = solver_data->points[b].position;
    const Vector3 line_direction = (b_pos - a_pos).unit();

    furthest_distance = -1.f;

    for (int i = 0; i < solver_data->points.size(); i++) {
        if (i == a || i == b)
            continue;

        const Vector3& point = solver_data->points[i].position;

        const Vector3 direction = point - a_pos;
        const float distance =
            (direction - line_direction * direction.dot(line_direction))
                .magnitude();

        if (distance > furthest_distance) {
            c = i;
            furthest_distance = distance;
        }
    }

    assert(c != -1);

    // Find the point furthest from the plane created by a --> b --> c.
    const Vector3& c_pos = solver_data->points[c].position;
    const Vector3 normal = (c_pos - a_pos).cross(b_pos - a_pos).unit();

    furthest_distance = 0.0f;

    for (int i = 0; i < solver_data->points.size(); i++) {
        if (i == a || i == b || i == c)
            continue;

        const Vector3& point = solver_data->points[i].position;
        const float distance = (point - a_pos).dot(normal);

        if (fabs(distance) > fabsf(furthest_distance)) {
            d = i;
            furthest_distance = distance;
        }
    }

    assert(d != -1);

    // Create my initial hull, which is a tetrahedron.
    if (furthest_distance > 0) {
        solver_data->faces.push_back(QuickHullFace(a, c, b, 2, 3, 1)); // 0
        solver_data->faces.push_back(QuickHullFace(a, d, c, 2, 0, 3)); // 1
        solver_data->faces.push_back(QuickHullFace(c, d, b, 3, 0, 1)); // 2
        solver_data->faces.push_back(QuickHullFace(b, d, a, 1, 0, 2)); // 3
    } else {
        solver_data->faces.push_back(QuickHullFace(a, b, c, 2, 1, 3)); // 0
        solver_data->faces.push_back(QuickHullFace(a, c, d, 2, 3, 0)); // 1
        solver_data->faces.push_back(QuickHullFace(c, b, d, 3, 1, 0)); // 2
        solver_data->faces.push_back(QuickHullFace(b, a, d, 1, 2, 0)); // 3
    }

    solver_data->points[a].face = INSIDE;
    solver_data->points[b].face = INSIDE;
    solver_data->points[c].face = INSIDE;
    solver_data->points[d].face = INSIDE;
}

// ReassignPointsToFaces:
// Assigns each QuickHull point to the face it is above, or inside if it is
// inside of the convex hull. After assignment, returns the point that is
// furthest from its corresponding face.
int QuickHullSolver::reassignPointsToFaces() {
    std::vector<QuickHullPoint>& points = solver_data->points;
    std::vector<QuickHullFace>& faces = solver_data->faces;

    int furthest_point = -1;
    float furthest_distance = -1.f;

    for (int i = 0; i < points.size(); i++) {
        QuickHullPoint& point = points[i];

        // Ignore the point if already inside the convex hull
        if (point.face == INSIDE)
            continue;

        point.face = UNASSIGNED;

        // Otherwise, find the first face the point is outside of
        for (int face_index = 0; face_index < faces.size(); face_index++) {
            if (!faces[face_index].in_convex_hull)
                continue;

            const float distance = solver_data->signedDistanceTo(face_index, i);
            if (distance > EPSILON) {
                point.face = face_index;

                // Save point if it is the furthest point from its face
                if (distance > furthest_distance) {
                    furthest_point = i;
                    furthest_distance = distance;
                }

                break;
            }
        }

        if (point.face == UNASSIGNED)
            point.face = INSIDE;
    }

    return furthest_point;
}

// FindHorizonEdge:
// Finds the horizon edge for a point using a DFS traversal.
void QuickHullSolver::findHorizonEdge(int point, int face, int prev_face) {
    QuickHullFace& cur_face = solver_data->faces[face];

    // Do not process if already traversed
    if (cur_face.traversal_flag)
        return;
    cur_face.traversal_flag = true;

    // Attempt to traverse to neighbors.
    // - If neighboring face is visible from point, then the edge we just
    //   crossed is not part of the horizon edge. Continue traversal to the
    //   neighbor.
    // - If neighboring face is not visible from point, then the edge we
    //   just crossed is part of the horizon edge. End traversal and add to
    //   vector.
    int edge_to_traverse = 0;

    // If we traversed to another face (prev_face != -1), we need to continue
    // our traversal starting from the next immediate counter-clockwise vertex
    // after the one opposite the face we came from.
    if (prev_face != -1) {
        if (prev_face == cur_face.i_opposite_faces[0])
            edge_to_traverse = 1;
        else if (prev_face == cur_face.i_opposite_faces[1])
            edge_to_traverse = 2;
        else if (prev_face == cur_face.i_opposite_faces[2])
            edge_to_traverse = 0;
        // This should never happen, but if somehow our previous face can't be
        // found, exist, our convex hull is degenerate.
        else
            assert(false); // Not possible
    }

    for (int i = 0; i < 3; i++) {
        const int index_0 = edge_to_traverse;
        const int index_1 = (edge_to_traverse + 1) % 3;
        const int index_2 = (edge_to_traverse + 2) % 3;

        // Checck if the next face is visible by the point.
        if (solver_data->signedDistanceTo(cur_face.i_opposite_faces[index_0],
                                          point) > EPSILON / 2) {
            findHorizonEdge(point, cur_face.i_opposite_faces[index_0], face);
        }
        // If it is not, then the edge we tried to cross is part of the horizon
        // edge-- add it to our vector. Because of our order of iteration, we
        // can guarantee that adding our edge now will maintain the proper CW
        // order.
        else {
            solver_data->horizon_edge.push_back(HorizonEdge(
                cur_face.i_points[index_1], cur_face.i_points[index_2], face,
                cur_face.i_opposite_faces[index_0]));
        }

        edge_to_traverse = index_1;
    }
}

} // namespace Math
} // namespace Engine