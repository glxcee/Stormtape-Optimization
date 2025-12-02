// SPDX-FileCopyrightText: 2025 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#ifndef STORM_UUID_GENERATOR_HPP
#define STORM_UUID_GENERATOR_HPP

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace storm {

class UuidGenerator
{
  boost::uuids::random_generator m_gen;

 public:
  std::string operator()()
  {
    return boost::uuids::to_string(m_gen());
  }
};

} // namespace storm

#endif
