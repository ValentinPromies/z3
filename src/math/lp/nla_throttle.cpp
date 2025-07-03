/*++
  Copyright (c) 2017 Microsoft Corporation

  Author:
    Lev Nachmanson (levnach)

  --*/
#include "math/lp/nla_throttle.h"
#include "util/trace.h"

namespace nla {

bool nla_throttle::insert_new(throttle_kind k, lpvar mvar, bool is_lt) {
    signature sig;
    sig.m_values[0] = static_cast<unsigned>(k);
    sig.m_values[1] = static_cast<unsigned>(mvar);
    sig.m_values[2] = static_cast<unsigned>(is_lt ? 1 : 0);
    return insert_new_impl(sig);
}

bool nla_throttle::insert_new(throttle_kind k, lpvar xy_var, lpvar x, lpvar y, int sign, int sy) {
    signature sig;
    sig.m_values[0] = static_cast<unsigned>(k);
    sig.m_values[1] = static_cast<unsigned>(xy_var);
    sig.m_values[2] = static_cast<unsigned>(x);
    sig.m_values[3] = static_cast<unsigned>(y);
    sig.m_values[4] = normalize_sign(sign);
    sig.m_values[5] = normalize_sign(sy);
    return insert_new_impl(sig);
}

bool nla_throttle::insert_new(throttle_kind k, lpvar ac_var, lpvar a, const rational& c_sign, lpvar c,
                              lpvar bd_var, lpvar b_var, const rational& d_sign, lpvar d, llc ab_cmp) {
    signature sig;
    sig.m_values[0] = static_cast<unsigned>(k);
    sig.m_values[1] = static_cast<unsigned>(ac_var);
    sig.m_values[2] = static_cast<unsigned>(a);
    sig.m_values[3] = pack_rational_sign(c_sign);
    sig.m_values[4] = static_cast<unsigned>(c);
    sig.m_values[5] = static_cast<unsigned>(bd_var);
    sig.m_values[6] = static_cast<unsigned>(b_var);
    // Pack d_sign, d, and ab_cmp into the last slot
    sig.m_values[7] = (pack_rational_sign(d_sign) << 24) | 
                     ((static_cast<unsigned>(d) & 0xFFFF) << 8) |
                     (static_cast<unsigned>(ab_cmp) & 0xFF);
    return insert_new_impl(sig);
}

bool nla_throttle::insert_new(throttle_kind k, lpvar monic_var, lpvar x_var, lpvar y_var, bool below, int plane_type) {
    signature sig;
    sig.m_values[0] = static_cast<unsigned>(k);
    sig.m_values[1] = static_cast<unsigned>(monic_var);
    sig.m_values[2] = static_cast<unsigned>(x_var);
    sig.m_values[3] = static_cast<unsigned>(y_var);
    sig.m_values[4] = static_cast<unsigned>(below ? 1 : 0);
    sig.m_values[5] = static_cast<unsigned>(plane_type);
    return insert_new_impl(sig);
}

bool nla_throttle::insert_new(throttle_kind k, lpvar monic_var, lpvar x_var, lpvar y_var, bool below) {
    signature sig;
    sig.m_values[0] = static_cast<unsigned>(k);
    sig.m_values[1] = static_cast<unsigned>(monic_var);
    sig.m_values[2] = static_cast<unsigned>(x_var);
    sig.m_values[3] = static_cast<unsigned>(y_var);
    sig.m_values[4] = static_cast<unsigned>(below ? 1 : 0);
    // No plane_type parameter, so leave m_values[5] as 0
    return insert_new_impl(sig);
}

bool nla_throttle::insert_new_powers(throttle_kind k, lpvar r, lpvar x, lpvar y, unsigned lemma_type) {
    signature sig;
    sig.m_values[0] = static_cast<unsigned>(k);
    sig.m_values[1] = static_cast<unsigned>(r);
    sig.m_values[2] = static_cast<unsigned>(x);
    sig.m_values[3] = static_cast<unsigned>(y);
    sig.m_values[4] = static_cast<unsigned>(lemma_type);
    return insert_new_impl(sig);
}

bool nla_throttle::insert_new_division(throttle_kind k, lpvar q, lpvar x, lpvar y, unsigned lemma_type) {
    signature sig;
    sig.m_values[0] = static_cast<unsigned>(k);
    sig.m_values[1] = static_cast<unsigned>(q);
    sig.m_values[2] = static_cast<unsigned>(x);
    sig.m_values[3] = static_cast<unsigned>(y);
    sig.m_values[4] = static_cast<unsigned>(lemma_type);
    return insert_new_impl(sig);
}

bool nla_throttle::insert_new_grobner(throttle_kind k, lpvar v1, lpvar v2, unsigned eq_id) {
    signature sig;
    sig.m_values[0] = static_cast<unsigned>(k);
    sig.m_values[1] = static_cast<unsigned>(v1);
    sig.m_values[2] = static_cast<unsigned>(v2);
    sig.m_values[3] = static_cast<unsigned>(eq_id);
    return insert_new_impl(sig);
}

bool nla_throttle::insert_new_basic_sign(throttle_kind k, lpvar m, int sign) {
    signature sig;
    sig.m_values[0] = static_cast<unsigned>(k);
    sig.m_values[1] = static_cast<unsigned>(m);
    sig.m_values[2] = normalize_sign(sign);
    return insert_new_impl(sig);
}

bool nla_throttle::insert_new_horner(throttle_kind k, lpvar v, unsigned term_id) {
    signature sig;
    sig.m_values[0] = static_cast<unsigned>(k);
    sig.m_values[1] = static_cast<unsigned>(v);
    sig.m_values[2] = static_cast<unsigned>(term_id);
    return insert_new_impl(sig);
}

bool nla_throttle::insert_new_factor(throttle_kind k, lpvar monic, lpvar factor, bool is_zero) {
    signature sig;
    sig.m_values[0] = static_cast<unsigned>(k);
    sig.m_values[1] = static_cast<unsigned>(monic);
    sig.m_values[2] = static_cast<unsigned>(factor);
    sig.m_values[3] = static_cast<unsigned>(is_zero ? 1 : 0);
    return insert_new_impl(sig);
}

bool nla_throttle::insert_new_impl(const signature& sig) {
    throttle_kind k = static_cast<throttle_kind>(sig.m_values[0]);
    
    // Check permanent throttling first (forbidden for life)
    if (m_permanently_forbidden.contains(sig)) {
        TRACE(nla_throttle, 
              tout << "permanently throttled lemma generation, kind=" << static_cast<int>(k) 
                   << " sig=[" << sig.m_values[0] << "," << sig.m_values[1] << "," 
                   << sig.m_values[2] << "," << sig.m_values[3] << "," 
                   << sig.m_values[4] << "," << sig.m_values[5] << "," 
                   << sig.m_values[6] << "," << sig.m_values[7] << "]\n";);
        m_stats.m_nla_throttled_lemmas++;
        
        // Update specific lemma type statistics
        switch (k) {
            case ORDER_LEMMA:
                m_stats.m_nla_throttled_order_lemmas++;
                break;
            case BINOMIAL_SIGN_LEMMA:
                m_stats.m_nla_throttled_binomial_sign_lemmas++;
                break;
            case MONOTONE_LEMMA:
                m_stats.m_nla_throttled_monotone_lemmas++;
                break;
            case TANGENT_LEMMA:
                m_stats.m_nla_throttled_tangent_lemmas++;
                break;
            case BASIC_SIGN_LEMMA:
                m_stats.m_nla_throttled_basic_sign_lemmas++;
                break;
            case POWERS_LEMMA:
                m_stats.m_nla_throttled_powers_lemmas++;
                break;
            case DIVISION_LEMMA:
                m_stats.m_nla_throttled_division_lemmas++;
                break;
            case GROBNER_LEMMA:
                m_stats.m_nla_throttled_grobner_lemmas++;
                break;
            case HORNER_LEMMA:
                m_stats.m_nla_throttled_horner_lemmas++;
                break;
            case FACTOR_ZERO_LEMMA:
                m_stats.m_nla_throttled_factor_zero_lemmas++;
                break;
            case FACTOR_NEUTRAL_LEMMA:
                m_stats.m_nla_throttled_factor_neutral_lemmas++;
                break;
            default:
                TRACE(nla_throttle, tout << "Unexpected throttle kind: " << static_cast<int>(k) << "\n";);
        }
        
        return true;  // Permanently throttled
    }
    
    // Check temporary throttling (backtrackable)
    if (m_seen.contains(sig)) {
        TRACE(nla_throttle, tout << "throttled lemma generation\n";);
        m_stats.m_nla_throttled_lemmas++;
        
        // Increment throttle count
        auto it = m_throttle_count.find_core(sig);
        unsigned count = (it != nullptr) ? it->get_data().m_value + 1 : 1;
        m_throttle_count.insert(sig, count);
        
        TRACE(nla_throttle, 
              tout << "throttled lemma, kind=" << static_cast<int>(k) << " count=" << count
                   << " sig=[" << sig.m_values[0] << "," << sig.m_values[1] << "," 
                   << sig.m_values[2] << "," << sig.m_values[3] << "," 
                   << sig.m_values[4] << "," << sig.m_values[5] << "," 
                   << sig.m_values[6] << "," << sig.m_values[7] << "]\n";);
        
        // Check if this lemma should be permanently banned
        if (should_permanently_ban(sig, k)) {
            m_permanently_forbidden.insert(sig);
            TRACE(nla_throttle, 
                  tout << "banned lemma for life, kind=" << static_cast<int>(k) 
                       << " after " << count << " throttles\n";);
        }
        
        // Update specific lemma type statistics
        switch (k) {
            case ORDER_LEMMA:
                m_stats.m_nla_throttled_order_lemmas++;
                break;
            case BINOMIAL_SIGN_LEMMA:
                m_stats.m_nla_throttled_binomial_sign_lemmas++;
                break;
            case MONOTONE_LEMMA:
                m_stats.m_nla_throttled_monotone_lemmas++;
                break;
            case TANGENT_LEMMA:
                m_stats.m_nla_throttled_tangent_lemmas++;
                break;
            case BASIC_SIGN_LEMMA:
                m_stats.m_nla_throttled_basic_sign_lemmas++;
                break;
            case POWERS_LEMMA:
                m_stats.m_nla_throttled_powers_lemmas++;
                break;
            case DIVISION_LEMMA:
                m_stats.m_nla_throttled_division_lemmas++;
                break;
            case GROBNER_LEMMA:
                m_stats.m_nla_throttled_grobner_lemmas++;
                break;
            case HORNER_LEMMA:
                m_stats.m_nla_throttled_horner_lemmas++;
                break;
            case FACTOR_ZERO_LEMMA:
                m_stats.m_nla_throttled_factor_zero_lemmas++;
                break;
            case FACTOR_NEUTRAL_LEMMA:
                m_stats.m_nla_throttled_factor_neutral_lemmas++;
                break;
            default:
                TRACE(nla_throttle, tout << "Unexpected throttle kind: " << static_cast<int>(k) << "\n";);
        }
        
        return true;  // Already seen, throttle
    }
    
    TRACE(nla_throttle, 
          tout << "new lemma, kind=" << static_cast<int>(k) 
               << " sig=[" << sig.m_values[0] << "," << sig.m_values[1] << "," 
               << sig.m_values[2] << "," << sig.m_values[3] << "," 
               << sig.m_values[4] << "," << sig.m_values[5] << "," 
               << sig.m_values[6] << "," << sig.m_values[7] << "]\n";);
    
    // Add to temporary throttling (backtrackable)
    m_seen.insert(sig);
    m_trail.push(insert_map(m_seen, sig));
    return false;     // New, don't throttle
}

} // namespace nla
