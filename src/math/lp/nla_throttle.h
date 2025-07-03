/*++
  Copyright (c) 2017 Microsoft Corporation

  Author:
    Lev Nachmanson (levnach)

  --*/
#pragma once
#include "math/lp/nla_defs.h"
#include "math/lp/lp_settings.h"
#include "util/hashtable.h"
#include "util/map.h"
#include "util/trail.h"
#include <cstring>

namespace nla {

class nla_throttle {
public:
    enum throttle_kind {
        ORDER_LEMMA,            // order lemma (9 params)
        BINOMIAL_SIGN_LEMMA,    // binomial sign (6 params) 
        MONOTONE_LEMMA,         // monotonicity (2 params)
        TANGENT_LEMMA,          // tangent lemma (5 params: monic_var, x_var, y_var, below, plane_type)
        BASIC_SIGN_LEMMA,       // basic sign lemmas
        POWERS_LEMMA,           // x^y constraint lemmas
        DIVISION_LEMMA,         // q = x/y constraint lemmas
        GROBNER_LEMMA,          // grobner basis lemmas
        HORNER_LEMMA,           // horner polynomial evaluation lemmas
        FACTOR_ZERO_LEMMA,      // lemma for mon zero factors
        FACTOR_NEUTRAL_LEMMA    // lemma for neutral monomial factors
    };

private:
    struct signature {
        unsigned m_values[8];
        
        signature() { 
            std::memset(m_values, 0, sizeof(m_values)); 
        }
        
        bool operator==(const signature& other) const {
            return std::memcmp(m_values, other.m_values, sizeof(m_values)) == 0;
        }
    };
    
    struct signature_hash {
        unsigned operator()(const signature& s) const {
            unsigned hash = 0;
            for (int i = 0; i < 8; i++) {
                hash = combine_hash(hash, s.m_values[i]);
            }
            return hash;
        }
    };
    
    // Temporary throttling (backtrackable via trail)
    hashtable<signature, signature_hash, default_eq<signature>> m_seen;
    
    // Permanent throttling (forbidden for life, not backtrackable)
    hashtable<signature, signature_hash, default_eq<signature>> m_permanently_forbidden;
    
    // Throttle frequency counter: tracks how many times each signature has been throttled
    map<signature, unsigned, signature_hash, default_eq<signature>> m_throttle_count;
    
    trail_stack& m_trail;
    lp::statistics& m_stats;
    
    // Throttling thresholds per lemma type
    unsigned get_permanent_throttle_threshold(throttle_kind k) const {
        switch (k) {
            case BASIC_SIGN_LEMMA:     return 2;  // Ban after 2 throttles - deterministic patterns
            case FACTOR_ZERO_LEMMA:    return 1;  // Ban after 1 throttle - highly deterministic
            case FACTOR_NEUTRAL_LEMMA: return 1;  // Ban after 1 throttle - highly deterministic
            case POWERS_LEMMA:         return 3;  // Ban after 3 throttles - some variability
            case DIVISION_LEMMA:       return 3;  // Ban after 3 throttles - some variability
            case GROBNER_LEMMA:        return 5;  // Ban after 5 throttles - context dependent
            case HORNER_LEMMA:         return 4;  // Ban after 4 throttles - moderately variable
            case ORDER_LEMMA:          return 4;  // Ban after 4 throttles - context dependent
            case BINOMIAL_SIGN_LEMMA:  return 3;  // Ban after 3 throttles - some variability
            case MONOTONE_LEMMA:       return 3;  // Ban after 3 throttles - context dependent
            case TANGENT_LEMMA:        return 4;  // Ban after 4 throttles - geometric constraints
            default:                   return 3;  // Conservative default
        }
    }
    
    // Check if a lemma should be permanently banned based on throttle count
    bool should_permanently_ban(const signature& sig, throttle_kind k) {
        unsigned threshold = get_permanent_throttle_threshold(k);
        auto it = m_throttle_count.find_core(sig);
        return it != nullptr && it->get_data().m_value >= threshold;
    }

public:
    nla_throttle(trail_stack& trail, lp::statistics& stats) : m_trail(trail), m_stats(stats) {}
    
    // Monotone lemma: mvar + is_lt
    bool insert_new(throttle_kind k, lpvar mvar, bool is_lt);
    
    // Binomial sign: xy_var + x + y + sign + sy
    bool insert_new(throttle_kind k, lpvar xy_var, lpvar x, lpvar y, int sign, int sy);
    
    // Order lemma: ac_var + a + c_sign + c + bd_var + b_var + d_sign + d + ab_cmp
    bool insert_new(throttle_kind k, lpvar ac_var, lpvar a, const rational& c_sign, lpvar c,
                    lpvar bd_var, lpvar b_var, const rational& d_sign, lpvar d, llc ab_cmp);
    
    // Tangent lemma: monic_var + x_var + y_var + below + plane_type
    bool insert_new(throttle_kind k, lpvar monic_var, lpvar x_var, lpvar y_var, bool below, int plane_type);
    
    // Tangent lemma (simplified): monic_var + x_var + y_var + below
    bool insert_new(throttle_kind k, lpvar monic_var, lpvar x_var, lpvar y_var, bool below);
    
    // Powers lemma: r + x + y + lemma_type
    bool insert_new_powers(throttle_kind k, lpvar r, lpvar x, lpvar y, unsigned lemma_type);
    
    // Division lemma: q + x + y + lemma_type
    bool insert_new_division(throttle_kind k, lpvar q, lpvar x, lpvar y, unsigned lemma_type);
    
    // Grobner lemma: variables with equation id
    bool insert_new_grobner(throttle_kind k, lpvar v1, lpvar v2, unsigned eq_id);
    
    // Basic sign lemma: m + sign
    bool insert_new_basic_sign(throttle_kind k, lpvar m, int sign);
    
    // Horner lemma: variable + term
    bool insert_new_horner(throttle_kind k, lpvar v, unsigned term_id);
    
    // Factor zero/neutral lemma: monic + factor
    bool insert_new_factor(throttle_kind k, lpvar monic, lpvar factor, bool is_zero);

private:
    bool insert_new_impl(const signature& sig);
    
    // Helper functions for packing values
    static unsigned pack_rational_sign(const rational& r) {
        return r.is_pos() ? 1 : (r.is_neg() ? 255 : 0);
    }
    
    static unsigned normalize_sign(int sign) {
        return static_cast<unsigned>(sign + 127);
    }
    
};

}
