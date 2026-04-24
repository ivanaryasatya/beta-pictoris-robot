#pragma once

#include <Arduino.h>

// ==================== AUTO SEQUENCE CONFIG ====================
struct SequenceStep {
    const char* command;
    unsigned long duration;
};

// Sequence 1: Maju, Mundur
const SequenceStep seq1[] = {
    {"rbt.move.0.100.0.0", 5000}, 
    {"rbt.move.0.0.0.0", 1000},   
    {"rbt.move.0.-100.0.0", 5000},
    {"rbt.move.0.0.0.0", 1000}    
};

// Sequence 2: Maju, Mundur, Kanan, Kiri (sesuai contoh)
const SequenceStep seq2[] =  {
    {"rbt.move.0.100.0.0", 2000},
    {"rbt.move.0.-100.0.0", 2000},
    {"rbt.move.100.0.0.0", 2000}, 
    {"rbt.move.-100.0.0.0", 2000},
    {"rbt.move.0.0.0.0", 500}
};

struct SequenceDef {
    const SequenceStep* steps;
    byte length;
};

const SequenceDef availableSequences[] = {
    {seq1, sizeof(seq1) / sizeof(seq1[0])},
    {seq2, sizeof(seq2) / sizeof(seq2[0])}
};

const byte totalSequences = sizeof(availableSequences) / sizeof(availableSequences[0]);