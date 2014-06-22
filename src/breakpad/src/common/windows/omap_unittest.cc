// Copyright 2013 Google Inc. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Unittests for OMAP related functions.

#include "common/windows/omap.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace google_breakpad {

// Equality operators for ContainerEq. These must be outside of the anonymous
// namespace in order for them to be found.
bool operator==(const MappedRange& mr1, const MappedRange& mr2) {
  return mr1.rva_original == mr2.rva_original &&
      mr1.rva_transformed == mr2.rva_transformed &&
      mr1.length == mr2.length &&
      mr1.injected == mr2.injected &&
      mr1.removed == mr2.removed;
}
bool operator==(const EndpointIndex& ei1, const EndpointIndex& ei2) {
  return ei1.endpoint == ei2.endpoint && ei1.index == ei2.index;
}

// Pretty printers for more meaningful error messages. Also need to be outside
// the anonymous namespace.
std::ostream& operator<<(std::ostream& os, const MappedRange& mr) {
  os << "MappedRange(rva_original=" << mr.rva_original
     << ", rva_transformed=" << mr.rva_transformed
     << ", length=" << mr.length
     << ", injected=" << mr.injected
     << ", removed=" << mr.removed << ")";
  return os;
}
std::ostream& operator<<(std::ostream& os, const EndpointIndex& ei) {
  os << "EndpointIndex(endpoint=" << ei.endpoint
     << ", index=" << ei.index << ")";
  return os;
}
std::ostream& operator<<(std::ostream& os, const AddressRange& ar) {
  os << "AddressRange(rva=" << ar.rva << ", length=" << ar.length << ")";
  return os;
}

namespace {

OMAP CreateOmap(DWORD rva, DWORD rvaTo) {
  OMAP o = { rva, rvaTo };
  return o;
}

MappedRange CreateMappedRange(DWORD rva_original,
                              DWORD rva_transformed,
                              DWORD length,
                              DWORD injected,
                              DWORD removed) {
  MappedRange mr = { rva_original, rva_transformed, length, injected, removed };
  return mr;
}

EndpointIndex CreateEndpointIndex(DWORD endpoint, size_t index) {
  EndpointIndex ei = { endpoint, index };
  return ei;
}

//              (C is removed)
// Original   :  A B C D E F G H
// Transformed:  A B D F E * H1 G1 G2 H2
//              (* is injected, G is copied, H is split)
// A is implied.

// Layout of the original image.
const AddressRange B(100, 15);
const AddressRange C(B.end(), 10);
const AddressRange D(C.end(), 25);
const AddressRange E(D.end(), 10);
const AddressRange F(E.end(), 40);
const AddressRange G(F.end(), 3);
const AddressRange H(G.end(), 7);

// Layout of the transformed image.
const AddressRange Bt(100, 15);
const AddressRange Dt(Bt.end(), 20);  // D is shortened.
const AddressRange Ft(Dt.end(), F.length);
const AddressRange Et(Ft.end(), E.length);
const AddressRange injected(Et.end(), 5);
const AddressRange H1t(injected.end(), 4);  // H is split.
const AddressRange G1t(H1t.end(), G.length);  // G is copied.
const AddressRange G2t(G1t.end(), G.length);  // G is copied.
const AddressRange H2t(G2t.end(), 3);  // H is split.

class BuildImageMapTest : public testing::Test {
 public:
  static const DWORD kInvalidAddress = 0xFFFFFFFF;

  void InitOmapData() {
    omap_data.length_original = H.end();

    // Build the OMAPTO vector (from transformed to original).
    omap_data.omap_to.push_back(CreateOmap(Bt.rva, B.rva));
    omap_data.omap_to.push_back(CreateOmap(Dt.rva, D.rva));
    omap_data.omap_to.push_back(CreateOmap(Ft.rva, F.rva));
    omap_data.omap_to.push_back(CreateOmap(Et.rva, E.rva));
    omap_data.omap_to.push_back(CreateOmap(injected.rva, kInvalidAddress));
    omap_data.omap_to.push_back(CreateOmap(H1t.rva, H.rva));
    omap_data.omap_to.push_back(CreateOmap(G1t.rva, G.rva));
    omap_data.omap_to.push_back(CreateOmap(G2t.rva, G.rva));
    omap_data.omap_to.push_back(CreateOmap(H2t.rva, H.rva + H1t.length));
    omap_data.omap_to.push_back(CreateOmap(H2t.end(), kInvalidAddress));

    // Build the OMAPFROM vector (from original to transformed).
    omap_data.omap_from.push_back(CreateOmap(B.rva, Bt.rva));
    omap_data.omap_from.push_back(CreateOmap(C.rva, kInvalidAddress));
    omap_data.omap_from.push_back(CreateOmap(D.rva, Dt.rva));
    omap_data.omap_from.push_back(CreateOmap(E.rva, Et.rva));
    omap_data.omap_from.push_back(CreateOmap(F.rva, Ft.rva));
    omap_data.omap_from.push_back(CreateOmap(G.rva, G1t.rva));
    omap_data.omap_from.push_back(CreateOmap(H.rva, H1t.rva));
    omap_data.omap_from.push_back(CreateOmap(H.rva + H1t.length, H2t.rva));
    omap_data.omap_from.push_back(CreateOmap(H.end(), kInvalidAddress));
  }

  OmapData omap_data;
};

}  // namespace

TEST_F(BuildImageMapTest, EmptyImageMapOnEmptyOmapData) {
  ASSERT_EQ(0u, omap_data.omap_from.size());
  ASSERT_EQ(0u, omap_data.omap_to.size());
  ASSERT_EQ(0u, omap_data.length_original);

  ImageMap image_map;
  BuildImageMap(omap_data, &image_map);
  EXPECT_EQ(0u, image_map.mapping.size());
  EXPECT_EQ(0u, image_map.endpoint_index_map.size());
}

TEST_F(BuildImageMapTest, ImageMapIsCorrect) {
  InitOmapData();
  ASSERT_LE(0u, omap_data.omap_from.size());
  ASSERT_LE(0u, omap_data.omap_to.size());
  ASSERT_LE(0u, omap_data.length_original);

  ImageMap image_map;
  BuildImageMap(omap_data, &image_map);
  EXPECT_LE(9u, image_map.mapping.size());
  EXPECT_LE(9u, image_map.endpoint_index_map.size());

  Mapping mapping;
  mapping.push_back(CreateMappedRange(0, 0, B.rva, 0, 0));
  // C is removed, and it originally comes immediately after B.
  mapping.push_back(CreateMappedRange(B.rva, Bt.rva, B.length, 0, C.length));
  // D is shortened by a length of 5.
  mapping.push_back(CreateMappedRange(D.rva, Dt.rva, Dt.length, 0, 5));
  // The injected content comes immediately after E in the transformed image.
  mapping.push_back(CreateMappedRange(E.rva, Et.rva, E.length, injected.length,
                                      0));
  mapping.push_back(CreateMappedRange(F.rva, Ft.rva, F.length, 0, 0));
  // G is copied so creates two entries.
  mapping.push_back(CreateMappedRange(G.rva, G1t.rva, G.length, 0, 0));
  mapping.push_back(CreateMappedRange(G.rva, G2t.rva, G.length, 0, 0));
  // H is split, so create two entries.
  mapping.push_back(CreateMappedRange(H.rva, H1t.rva, H1t.length, 0, 0));
  mapping.push_back(CreateMappedRange(H.rva + H1t.length, H2t.rva, H2t.length,
                                      0, 0));
  EXPECT_THAT(mapping,
              testing::ContainerEq(image_map.mapping));

  EndpointIndexMap endpoint_index_map;
  endpoint_index_map.push_back(CreateEndpointIndex(0, 0));
  endpoint_index_map.push_back(CreateEndpointIndex(B.rva, 1));
  endpoint_index_map.push_back(CreateEndpointIndex(D.rva, 2));
  endpoint_index_map.push_back(CreateEndpointIndex(E.rva, 3));
  endpoint_index_map.push_back(CreateEndpointIndex(F.rva, 4));
  // G is duplicated so 2 ranges map back to it, hence the skip from 5 to 7.
  endpoint_index_map.push_back(CreateEndpointIndex(G.rva, 5));
  // H is split so we expect 2 endpoints to show up attributed to it.
  endpoint_index_map.push_back(CreateEndpointIndex(H.rva, 7));
  endpoint_index_map.push_back(CreateEndpointIndex(H.rva + H1t.length, 8));
  endpoint_index_map.push_back(CreateEndpointIndex(H.end(), 9));
  EXPECT_THAT(endpoint_index_map,
              testing::ContainerEq(image_map.endpoint_index_map));
}

namespace {

class MapAddressRangeTest : public BuildImageMapTest {
 public:
  typedef BuildImageMapTest Super;
  virtual void SetUp() {
    Super::SetUp();
    InitOmapData();
    BuildImageMap(omap_data, &image_map);
  }

  ImageMap image_map;

 private:
  using BuildImageMapTest::InitOmapData;
  using BuildImageMapTest::omap_data;
};

}  // namespace

TEST_F(MapAddressRangeTest, EmptyImageMapReturnsIdentity) {
  ImageMap im;
  AddressRangeVector mapped_ranges;
  AddressRange ar(0, 1024);
  MapAddressRange(im, ar, &mapped_ranges);
  EXPECT_EQ(1u, mapped_ranges.size());
  EXPECT_EQ(ar, mapped_ranges[0]);
}

TEST_F(MapAddressRangeTest, MapOutOfImage) {
  AddressRangeVector mapped_ranges;
  MapAddressRange(image_map, AddressRange(H.end() + 10, 10), &mapped_ranges);
  EXPECT_EQ(0u, mapped_ranges.size());
}

TEST_F(MapAddressRangeTest, MapIdentity) {
  AddressRangeVector mapped_ranges;
  MapAddressRange(image_map, B, &mapped_ranges);
  EXPECT_EQ(1u, mapped_ranges.size());
  EXPECT_THAT(mapped_ranges, testing::ElementsAre(B));
}

TEST_F(MapAddressRangeTest, MapReorderedContiguous) {
  AddressRangeVector mapped_ranges;

  AddressRange DEF(D.rva, F.end() - D.rva);
  MapAddressRange(image_map, DEF, &mapped_ranges);
  EXPECT_EQ(1u, mapped_ranges.size());

  AddressRange DFEt(Dt.rva, Et.end() - Dt.rva);
  EXPECT_THAT(mapped_ranges, testing::ElementsAre(DFEt));
}

TEST_F(MapAddressRangeTest, MapEmptySingle) {
  AddressRangeVector mapped_ranges;
  MapAddressRange(image_map, AddressRange(D.rva, 0), &mapped_ranges);
  EXPECT_EQ(1u, mapped_ranges.size());
  EXPECT_THAT(mapped_ranges, testing::ElementsAre(AddressRange(Dt.rva, 0)));
}

TEST_F(MapAddressRangeTest, MapEmptyCopied) {
  AddressRangeVector mapped_ranges;
  MapAddressRange(image_map, AddressRange(G.rva, 0), &mapped_ranges);
  EXPECT_EQ(2u, mapped_ranges.size());
  EXPECT_THAT(mapped_ranges, testing::ElementsAre(AddressRange(G1t.rva, 0),
                                                  AddressRange(G2t.rva, 0)));
}

TEST_F(MapAddressRangeTest, MapCopiedContiguous) {
  AddressRangeVector mapped_ranges;
  MapAddressRange(image_map, G, &mapped_ranges);
  EXPECT_EQ(1u, mapped_ranges.size());
  EXPECT_THAT(mapped_ranges, testing::ElementsAre(
      AddressRange(G1t.rva, G2t.end() - G1t.rva)));
}

TEST_F(MapAddressRangeTest, MapSplitDiscontiguous) {
  AddressRangeVector mapped_ranges;
  MapAddressRange(image_map, H, &mapped_ranges);
  EXPECT_EQ(2u, mapped_ranges.size());
  EXPECT_THAT(mapped_ranges, testing::ElementsAre(H1t, H2t));
}

TEST_F(MapAddressRangeTest, MapInjected) {
  AddressRangeVector mapped_ranges;

  AddressRange EFGH(E.rva, H.end() - E.rva);
  MapAddressRange(image_map, EFGH, &mapped_ranges);
  EXPECT_EQ(1u, mapped_ranges.size());

  AddressRange FEHGGHt(Ft.rva, H2t.end() - Ft.rva);
  EXPECT_THAT(mapped_ranges, testing::ElementsAre(FEHGGHt));
}

TEST_F(MapAddressRangeTest, MapRemovedEntirely) {
  AddressRangeVector mapped_ranges;
  MapAddressRange(image_map, C, &mapped_ranges);
  EXPECT_EQ(0u, mapped_ranges.size());
}

TEST_F(MapAddressRangeTest, MapRemovedPartly) {
  AddressRangeVector mapped_ranges;
  MapAddressRange(image_map, D, &mapped_ranges);
  EXPECT_EQ(1u, mapped_ranges.size());
  EXPECT_THAT(mapped_ranges, testing::ElementsAre(Dt));
}

TEST_F(MapAddressRangeTest, MapFull) {
  AddressRangeVector mapped_ranges;

  AddressRange AH(0, H.end());
  MapAddressRange(image_map, AH, &mapped_ranges);
  EXPECT_EQ(1u, mapped_ranges.size());

  AddressRange AHt(0, H2t.end());
  EXPECT_THAT(mapped_ranges, testing::ElementsAre(AHt));
}

}  // namespace google_breakpad
