// Copyright (C) 2009-2016 ETH Zurich
// Copyright (C) 2007?-2016 Center for Nanophase Materials Sciences, ORNL
// All rights reserved.
//
// See LICENSE.txt for terms of usage.
// See CITATION.txt for citation guidelines if you use this code for scientific publications.
//
// Author: Peter Staar (peter.w.j.staar@gmail.com)
//
// Description

#ifndef PHYS_LIBRARY_DOMAINS_QUANTUM_DOMAIN_ELECTRON_BAND_DOMAIN_H
#define PHYS_LIBRARY_DOMAINS_QUANTUM_DOMAIN_ELECTRON_BAND_DOMAIN_H

#include <cassert>
#include <string>
#include <vector>

struct band_element {
  band_element();

  int number;
  int flavor;
  std::vector<double> a_vec;
};

band_element::band_element() : number(-1), flavor(-1), a_vec(0, 0) {}

class electron_band_domain {
public:
  typedef band_element element_type;

public:
  static int& get_size();
  static std::string get_name();

  static std::vector<element_type>& get_elements();

  template <typename Reader>
  static void read(Reader& reader);

  template <typename Writer>
  static void write(Writer& writer);

  template <typename parameters_type>
  static void initialize(parameters_type& parameters, int Nb_bands, std::vector<int> flavors,
                         std::vector<std::vector<double>> a_vecs);

  template <class stream_type>
  static void to_JSON(stream_type& ss);

private:
  static std::vector<element_type>& initialize_elements();
};

int& electron_band_domain::get_size() {
  static int size = 0;
  return size;
}

std::string electron_band_domain::get_name() {
  static std::string name = "electron-band-domain";
  return name;
}

std::vector<electron_band_domain::element_type>& electron_band_domain::get_elements() {
  static std::vector<element_type> elements(get_size());
  return elements;
}

template <typename Writer>
void electron_band_domain::write(Writer& writer) {
  writer.open_group(get_name());
  writer.execute(get_elements());
  writer.close_group();
}

template <typename parameters_type>
void electron_band_domain::initialize(parameters_type& /*parameters*/, int NB_BANDS,
                                      std::vector<int> /*flavors*/,
                                      std::vector<std::vector<double>> a_vecs) {
  get_size() = NB_BANDS;

  // assert(NB_BANDS == int(flavors.size()));
  assert(NB_BANDS == int(a_vecs.size()));

  for (size_t i = 0; i < get_elements().size(); ++i) {
    get_elements()[i].number = i;
    get_elements()[i].flavor = i;  // flavors[i];
    get_elements()[i].a_vec = a_vecs[i];
  }
}

#endif  // PHYS_LIBRARY_DOMAINS_QUANTUM_DOMAIN_ELECTRON_BAND_DOMAIN_H
