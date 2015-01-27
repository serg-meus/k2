#include "chess.h"

// WARNING: change values may cause error in AppriceMoves()

SC pst[6][2][8][8] =

{
  {         // KING
    {       {-8,   -18,   -18,   -28,   -28,   -18,   -18,    -8},      // * 1.5
            {-8,   -18,   -18,   -28,   -28,   -18,   -18,    -8},
            {-8,   -18,   -18,   -28,   -28,   -18,   -18,    -8},
            {-8,   -18,   -18,   -28,   -28,   -18,   -18,    -8},
            { 2,    -8,    -8,   -18,   -18,    -8,    -8,     2},
            {12,     2,     2,     2,     2,     2,     2,    12},
            {42,    42,    22,    22,    22,    22,    42,    42},
            {42,    52,    32,    22,    22,    32,    52,    42}
    },

    {   {-60,   -45,   -31,   -16,   -16,   -31,   -45,   -60},         // * 0
        {-30,   -15,    -1,    14,    14,    -1,   -15,   -30},
        {-30,     0,    44,    59,    59,    44,     0,   -30},
        {-30,     0,    60,    75,    75,    60,     0,   -30},
        {-30,     0,    60,    75,    75,    60,     0,   -30},
        {-30,     0,    44,    59,    59,    44,     0,   -30},
        {-30,   -15,    -1,    14,    14,    -1,   -15,   -30},
        {-60,   -45,   -31,   -16,   -16,   -31,   -45,   -60}
    }
  },

  {         // QUEEN
    {   {-16,    -6,    -6,    -1,    -1,    -6,    -6,   -16},
        { -6,     4,     4,     4,     4,     4,     4,    -6},
        { -6,     4,     4,     4,     4,     4,     4,    -6},
        { -1,     4,     4,     4,     4,     4,     4,    -1},
        { -1,     4,     4,     4,     4,     4,     4,    -1},
        { -6,     4,     4,     4,     4,     4,     4,    -6},
        { -6,     4,     4,     4,     4,     4,     4,    -6},
        {-16,    -6,    -6,    -1,    -1,    -6,    -6,   -16}
    },

    {   {-17,    -7,    -7,    -2,    -2,    -7,    -7,   -17},
        { -7,     3,     3,     3,     3,     3,     3,    -7},
        { -7,     3,     8,     8,     8,     8,     3,    -7},
        { -2,     3,     8,     8,     8,     8,     3,    -2},
        { -2,     3,     8,     8,     8,     8,     3,    -2},
        { -7,     3,     8,     8,     8,     8,     3,    -7},
        { -7,     3,     3,     3,     3,     3,     3,    -7},
        {-17,    -7,    -7,    -7,    -7,    -7,    -7,   -17},
    }
  },

  {      // ROOK
    {   { -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1},
        {  4,     9,     9,     9,     9,     9,     9,     4},
        { -6,    -1,    -1,    -1,    -1,    -1,    -1,    -6},
        { -6,    -1,    -1,    -1,    -1,    -1,    -1,    -6},
        { -6,    -1,    -1,    -1,    -1,    -1,    -1,    -6},
        { -6,    -1,    -1,    -1,    -1,    -1,    -1,    -6},
        { -6,    -1,    -1,    -1,    -1,    -1,    -1,    -6},
        { -1,    -1,    -1,     4,     4,    -1,    -1,    -1}
    },

    {   {  0,     0,     0,     0,     0,     0,     0,     0},
        { -5,     0,     0,     0,     0,     0,     0,    -5},
        { -5,     0,     0,     0,     0,     0,     0,    -5},
        { -5,     0,     0,     0,     0,     0,     0,    -5},
        { -5,     0,     0,     0,     0,     0,     0,    -5},
        { -5,     0,     0,     0,     0,     0,     0,    -5},
        { -5,     0,     0,     0,     0,     0,     0,    -5},
        {  0,     0,     0,     0,     0,     0,     0,     0}
    }
  },

  {      //BISHOP
    {   {-20,   -10,   -10,   -10,   -10,   -10,   -10,   -20},   // * 4.63
        {-10,     0,     0,     0,     0,     0,     0,   -10},
        {-10,     0,     5,    10,    10,     5,     0,   -10},
        {-10,     5,     5,    10,    10,     5,     5,   -10},
        {-10,     0,    10,    10,    10,    10,     0,   -10},
        {-10,    10,    10,    10,    10,    10,    10,   -10},
        {-10,     5,     0,     0,     0,     0,     5,   -10},
        {-20,   -10,   -10,   -10,   -10,   -10,   -10,   -20}
    },

    {   {-18,    -8,    -8,    -8,    -8,    -8,    -8,   -18},     // * 0.54
        { -8,     2,     2,     2,     2,     2,     2,    -8},
        { -8,     2,     7,    12,    12,     7,     2,    -8},
        { -8,     7,     7,    12,    12,     7,     7,    -8},
        { -8,     2,    12,    12,    12,    12,     2,    -8},
        { -8,    12,    12,    12,    12,    12,    12,    -8},
        { -8,     7,     2,     2,     2,     2,     7,    -8},
        {-18,    -8,    -8,    -8,    -8,    -8,    -8,   -18}
    }
  },

  {      //KNIGHT
    {   {-38,   -28,   -18,   -18,   -18,   -18,   -28,   -38},     // * 2.06
        {-28,    -8,    12,    12,    12,    12,    -8,   -28},
        {-18,    12,    22,    27,    27,    22,    12,   -18},
        {-18,    17,    27,    32,    32,    27,    17,   -18},
        {-18,    12,    27,    32,    32,    27,    12,   -18},
        {-18,    17,    22,    27,    27,    22,    17,   -18},
        {-28,    -8,    12,    17,    17,    12,    -8,   -28},
        {-38,   -28,   -18,   -18,   -18,   -18,   -28,   -38}
    },

    {   {-75,   -25,   -15,   -15,   -15,   -15,   -25,   -75},     // * 0.44
        {-25,    -5,    15,    15,    15,    15,    -5,   -25},
        {-15,    15,    25,    30,    30,    25,    15,   -15},
        {-15,    20,    30,    35,    35,    30,    20,   -15},
        {-15,    15,    30,    35,    35,    30,    15,   -15},
        {-15,    20,    25,    30,    30,    25,    20,   -15},
        {-25,    -5,    15,    20,    20,    15,    -5,   -25},
        {-75,   -25,   -15,   -15,   -15,   -15,   -25,   -75}
    }
  },

  {     //PAWN
    {   {-11,   -11,   -11,   -11,   -11,   -11,   -11,   -11},     // *1.25
        { 39,    39,    39,    39,    39,    39,    39,    39},
        { -1,    -1,     9,    19,    19,     9,    -1,    -1},
        { -6,    -6,    -1,    14,    14,    -1,    -6,    -6},
        {-11,   -11,   -11,     9,     9,   -11,   -11,   -11},
        { -6,   -16,   -21,   -11,   -11,   -21,   -16,    -6},
        { -6,    -1,    -1,   -31,   -31,    -1,    -1,    -6},
        {-11,   -11,   -11,   -11,   -11,   -11,   -11,   -11},
    },

    {   {-11,   -11,   -11,   -11,   -11,   -11,   -11,   -11},     // * 0.35
        { 39,    39,    39,    39,    39,    39,    39,    39},
        { -1,    -1,     9,    19,    19,     9,    -1,    -1},
        { -6,    -6,    -1,    14,    14,    -1,    -6,    -6},
        {-11,   -11,   -11,     9,     9,   -11,   -11,   -11},
        { -6,   -16,   -21,   -11,   -11,   -21,   -16,    -6},
        { -6,    -1,    -1,   -31,   -31,    -1,    -1,    -6},
        {-11,   -11,   -11,   -11,   -11,   -11,   -11,   -11},
    }
  }
};
