#pragma once

#define CAT(X,Y) X##_##Y
#define TEMPLATE(X, Y) CAT(X,Y)
#define TEMPLATE3(X, Y, Z) TEMPLATE(CAT(X, Y), Z)
