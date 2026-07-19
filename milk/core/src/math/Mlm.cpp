#include <core/math/IMlm.h>

namespace mk::mlm
{

namespace
{
inline const mat4 kIdentityMat;
}

// -------------------------------- Transform ---------------------------------
Transform Transform::scale(f32 f)
{
    vec3 vec(f);

    return Transform(mat4(glm::scale(kIdentityMat.data_, vec.data_)));
}

Transform Transform::scale(f32 x, f32 y, f32 z)
{
    return Transform(mat4(glm::scale(kIdentityMat.data_, { x, y, z })));
}

Transform Transform::translate(f32 x, f32 y, f32 z)
{
    return Transform(mat4(glm::translate(kIdentityMat.data_, { x, y, z })));
}

Transform Transform::rotate(quat q)
{
    return Transform(mat4(glm::mat4_cast(q.data_)));
}

Transform Transform::inverse(const Transform& t)
{
    return Transform(mat4::inverse(t.data_));
}

Transform Transform::operator*(const Transform& other)
{
    return Transform(this->data_ * other.data_);
}

mat4& Transform::getRaw()
{
    return data_;
}

Point Transform::operator*(const Point& pt)
{
    vec4 v = this->data_ * pt.data_;
    if (v.w() != 1.0f)
    {
        return Point((1.0f / v.w()) * v);
    }
    return Point(v);
}

Vector Transform::operator*(const Vector& vec)
{
    return Vector(this->data_ * vec.data_);
}

// ---------------------------------- Point -----------------------------------
Point::Point(const Vector& v): Point(v.x(), v.y(), v.z()) {}

f32& Point::x()
{
    return data_.x();
}

f32& Point::y()
{
    return data_.y();
}

f32& Point::z()
{
    return data_.z();
}

f32 Point::x() const
{
    return data_.x();
}

f32 Point::y() const
{
    return data_.y();
}

f32 Point::z() const
{
    return data_.z();
}

Vector Point::operator-(const Point& rhs) const
{
    vec4 res = data_ - rhs.data_;
    res.w() = 0;
    return Vector(res);
}

// ----------------------------------- Vector ------------------------------------

Vector cross(const Vector& p, const Vector& q)
{
    vec3 pvec3 = vec3(p.x(), p.y(), p.z());
    vec3 qvec3 = vec3(q.x(), q.y(), q.z());
    return Vector(vec3::cross(pvec3, qvec3));
}

Vector normalize(const Vector& vec)
{
    return vec4::normalize(vec.data_);
}

f32 dot(const Vector& p, const Vector& q)
{
    return vec4::dot(p.data_, q.data_);
}

f32 magnitudeSq(const Vector& v)
{
    return vec4::magnitudeSq(v.data_);
}

f32 magnitude(const Vector& v)
{
    return vec4::magnitude(v.data_);
}

f32 distanceSq(const Point& p, const Point& q)
{
    return vec4::distanceSq(p.data_, q.data_);
}

f32 distance(const Point& p, const Point& q)
{
    return vec4::distance(p.data_, q.data_);
}

Vector operator*(f32 d, const Vector& v)
{
    return d * v.data_;
}

f32& Vector::x()
{
    return data_.x();
}

f32& Vector::y()
{
    return data_.y();
}

f32& Vector::z()
{
    return data_.z();
}

f32 Vector::x() const
{
    return data_.x();
}

f32 Vector::y() const
{
    return data_.y();
}

f32 Vector::z() const
{
    return data_.z();
}

// --------------------------------- Quat --------------------------------- //
namespace details
{
f32& Quat::x()
{
    return data_.x;
}

f32& Quat::y()
{
    return data_.y;
}

f32& Quat::z()
{
    return data_.z;
}

f32& Quat::w()
{
    return data_.w;
}

f32 Quat::x() const
{
    return data_.x;
}

f32 Quat::y() const
{
    return data_.y;
}

f32 Quat::z() const
{
    return data_.z;
}

f32 Quat::w() const
{
    return data_.w;
}
} // namespace details

// --------------------------------- util --------------------------------- //

Point projectTo(const Point& p, const Vector& d)
{
    f32 mag = dot(Vector(p), d);
    return Point(mag * d);
}

f32 toFovY(f32 fovX, f32 aspect)
{
    return 2.0f * std::atan(std::tan(fovX * 0.5f) / aspect);
}

bool raySphereIntersection(const Ray& r, const Sphere& s, f32& outT)
{
    f32   mag = dot(Vector(s.center), r.d);
    Point projected = Point(mag * r.d);
    bool  res = distanceSq(projected, s.center) <= s.r * s.r;

    if (res)
    {
        outT = mag;
    }
    return res;
}

// TODO(Cheese_S): micro benchmark to see if it's worth to use simd
// Or, we should just keep it as Vec3s.
bool rayTriIntersection(const Ray& r, Point v0, Point v1, Point v2)
{
    constexpr f32 kEpislon = std::numeric_limits<f32>::epsilon();

    Vector e1 = Vector(v1 - v0);
    Vector e2 = Vector(v2 - v0);

    const Vector n = cross(e1, e2);
    if (dot(r.d, n) > 0)
    {
        return false;
    }

    Vector rayCrossE2 = cross(r.d, e2);
    f32    det = dot(e1, rayCrossE2);

    if (abs(det) < kEpislon)
    {
        return false;
    }

    f32    invDet = 1.0f / det;
    Vector s = r.o - v0;
    f32    u = dot(s, rayCrossE2) * invDet;

    if (u < -kEpislon || u > kEpislon + 1)
    {
        return false;
    }

    Vector sCrossE1 = cross(s, e1);
    f32    v = dot(r.d, sCrossE1) * invDet;

    // TODO(Cheese_S): missing intersection time
    return v >= -kEpislon && u + v - 1 <= kEpislon;
    // if (v < -kEpislon || u + v - 1 > kEpislon)
    // {
    //     return false;
    // }
    //
    // return true;
}

} // namespace mk::mlm
