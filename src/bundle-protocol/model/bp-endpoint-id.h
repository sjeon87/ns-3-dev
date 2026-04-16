/*
 * Copyright (c) 2013 University of New Brunswick
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *
 *
 * Author: Dizhi Zhou <dizhi.zhou@gmail.com>
 */

#ifndef BP_ENDPOINT_ID_H
#define BP_ENDPOINT_ID_H

#include <iostream>
#include <string>

namespace ns3
{

/**
 * @brief A data structure that stores a single std::string buffer containing
 * the whole URI (scheme:ssp) and offset/length pairs for the scheme and ssp.
 *
 * This structure is referred from oasys/util/URI.h in DTN2
 */
struct Component
{
    Component()
        : offset(0),
          length(0)
    {
    }

    size_t offset; /// string offset within endpoint id
    size_t length; /// string length
};

/**
 * @brief The endpoint id of bundle node.
 *
 * The format of the endpoint id is defined at the section 4.4 in RFC 5050. An endpoint id
 * of a bundle node is represented as a string "scheme:ssp"
 *
 * Part of methods in this class is referred from oasys/util/URI.h in DTN2
 */
class BpEndpointId
{
  public:
    /**
     * Build an empty URI
     */
    BpEndpointId()
        : m_uri("")
    {
    }

    /**
     * Build an URI as "scheme:ssp"
     *
     * @param scheme scheme string of endpoint id
     * @param ssp ssp string of endpoint id
     */
    BpEndpointId(const std::string scheme, const std::string ssp);

    /**
     * Build an URI as "scheme:ssp"
     *
     * @param uri uri string of endpoint id
     */
    BpEndpointId(const std::string uri);

    /**
     * Destroy
     */
    virtual ~BpEndpointId() {};

    /**
     * Return the scheme part of endpoint id
     *
     * @return the scheme part of endpoint id
     */
    std::string Scheme() const;

    /**
     * Return the ssp part of endpoint id
     *
     * @return the ssp part of endpoint id
     */
    std::string Ssp() const;

    /**
     * Return the full name (uri) of endpoint id
     *
     * @return the full name (uri) of endpoint id
     */
    std::string Uri() const;

  private:
    /**
     * Check the naming rules of scheme and ssp
     *
     * @param scheme scheme string of endpoint id
     * @param ssp ssp string of endpoint id
     */
    void ParseComponent(const std::string& scheme, const std::string& ssp);

    /**
     * Check the naming rule of full name (uri) of endpoint id
     *
     * @param uri full name of endpoint id
     */
    void ParseUri(const std::string uri);

    /**
     * @brief operator ==
     */
    friend bool operator==(const BpEndpointId& a, const BpEndpointId& b);

    /**
     * @brief operator !=
     */
    friend bool operator!=(const BpEndpointId& a, const BpEndpointId& b);

    /**
     * @brief operator <
     */
    friend bool operator<(const BpEndpointId& a, const BpEndpointId& b);

  private:
    std::string m_uri; /// the endpoint id is represented by an URI in BP protocol

    Component m_scheme; /// the offset and the length of scheme part of URI
    Component m_ssp;    /// the offset and the length of ssp part of URI
};

inline bool
operator==(const BpEndpointId& a, const BpEndpointId& b)
{
    return (a.m_uri == b.m_uri);
}

inline bool
operator!=(const BpEndpointId& a, const BpEndpointId& b)
{
    return (a.m_uri != b.m_uri);
}

inline bool
operator<(const BpEndpointId& a, const BpEndpointId& b)
{
    return (a.m_uri < b.m_uri);
}

} // namespace ns3

#endif /* BP_ENDPOINT_ID_H */
