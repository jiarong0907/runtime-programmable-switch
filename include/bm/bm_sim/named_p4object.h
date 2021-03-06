/* Copyright 2013-present Barefoot Networks, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

//! @file named_p4object.h

#ifndef BM_BM_SIM_NAMED_P4OBJECT_H_
#define BM_BM_SIM_NAMED_P4OBJECT_H_

#include <bm/bm_sim/source_info.h>
#include <memory>
#include <string>
#include <utility>

namespace bm {

using p4object_id_t = int;

//! NamedP4Object is used as a base class for all the bmv2 classes with are used
//! to represent named P4 objects (e.g. Parser for P4 `parser` objects). It
//! just stores the name of the P4 instance and a compiler-provided id, which is
//! different from the id of other objects of the same class.
class NamedP4Object {
 public:
  NamedP4Object(const std::string &name, p4object_id_t id)
    : name(name), id(id) {}
  NamedP4Object(const std::string &name, p4object_id_t id,
                std::unique_ptr<SourceInfo> source_info)
    : name(name), id(id), source_info(std::move(source_info)) {}

  virtual ~NamedP4Object() { }

  //! Get the name of the P4 instance
  const std::string &get_name() const { return name; }

  //! Get the compiler-provided id
  p4object_id_t get_id() const { return id; }

  //! Deleted copy constructor
  NamedP4Object(const NamedP4Object &other) = delete;
  //! Deleted copy assignment operator
  NamedP4Object &operator=(const NamedP4Object &other) = delete;

  //! Default move constructor
  NamedP4Object(NamedP4Object &&other) = default;
  //! Default assignment operator
  NamedP4Object &operator=(NamedP4Object &&other) = default;

  const SourceInfo *get_source_info() const { return source_info.get(); }

  void set_id(p4object_id_t _id) { id = _id; }

  void set_name(const std::string &_name) { name = _name; }

 protected:
  // FlexCore: We remove const constraint on name. 
  // TODO: is it safe?
  std::string name;
  p4object_id_t id;
  std::unique_ptr<SourceInfo> source_info;
};

}  // namespace bm

#endif  // BM_BM_SIM_NAMED_P4OBJECT_H_
