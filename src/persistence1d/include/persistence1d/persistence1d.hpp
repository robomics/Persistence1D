/*! \file persistence1d.hpp
    Actual code.
*/

#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <vector>

namespace p1d {

/** Used to sort data according to its absolute value and refer to its original index in the Data
   vector.

        A collection of TIdxAndData is sorted according to its data value (if values are equal,
   according to indices). The index allows access back to the vertex in the Data vector.
*/
struct TIdxAndData {
  /// The index of the vertex within the Data vector.
  std::int64_t Idx{-1};

  /// Vertex data value from the original Data vector sent as an argument to RunPersistence.
  double Data{0};

  constexpr bool operator<(const TIdxAndData& other) const noexcept {
    if (Data < other.Data) {
      return true;
    }
    if (Data > other.Data) {
      return false;
    }
    return Idx < other.Idx;
  }
};

/*! Defines a component within the data domain.
        A component is created at a local minimum - a vertex whose value is smaller than both of its
   neighboring vertices' values.
*/
struct TComponent {
  /// A component is defined by the indices of its edges.
  /// Both variables hold the respective indices of the vertices in Data vector.
  /// All vertices between them are considered to belong to this component.
  std::size_t LeftEdgeIndex;
  std::size_t RightEdgeIndex;

  /// The index of the local minimum within the component as longs as its alive.
  std::size_t MinIndex;

  /// The value of the Data[MinIndex].
  double MinValue;  // redundant, but makes life easier

  /// Set to true when a component is created. Once components are merged,
  /// the destroyed component Alive value is set to false.
  /// Used to verify correctness of algorithm.
  bool Alive;
};

/** A pair of matched local minimum and local maximum
        that define a component above a certain persistence threshold.
        The persistence value is their (absolute) data difference.
*/
struct TPairedExtrema {
  /// Index of local minimum, as per Data vector.
  std::size_t MinIndex;

  /// Index of local maximum, as per Data vector.
  std::size_t MaxIndex;

  /// The persistence of the two extrema.
  /// Data[MaxIndex] - Data[MinIndex]
  /// Guaranteed to be >= 0.
  double Persistence;

  constexpr bool operator<(const TPairedExtrema& other) const noexcept {
    if (Persistence < other.Persistence) {
      return true;
    }
    if (Persistence > other.Persistence) {
      return false;
    }
    return MinIndex < other.MinIndex;
  }
};

/*! Finds extrema and their persistence in one-dimensional data.

        Local minima and local maxima are extracted, paired,
        and sorted according to their persistence.
        The global minimum is extracted as well.

        We assume a connected one-dimensional domain.
        Think of "data on a line", or a function f(x) over some domain xmin <= x <= xmax.
*/
class Persistence1D {
  static constexpr std::int64_t NO_COLOR = -1;
  static constexpr std::size_t RESIZE_FACTOR = 20;
  /*!
          Contain a copy of the original input data.
  */
  std::vector<double> _data{};

  /*!
          Contains a copy the value and index pairs of Data, sorted according to the data values.
  */
  std::vector<TIdxAndData> _sorted_data{};

  /*!
          Contains the Component assignment for each vertex in Data.
          Only edges of destroyed components are updated to the new component color.
          The Component values in this vector are invalid at the end of the algorithm.
  */
  std::vector<std::int64_t> _colors{};  // need to init to empty

  /*!
          A vector of Components.
          The component index within the vector is used as its Colors in the Watershed function.
  */
  std::vector<TComponent> _components{};

  /*!
          A vector of paired extrema features - always a minimum and a maximum.
  */
  std::vector<TPairedExtrema> _poaired_extrema{};

  // keeps track of component vector size and newest component "color"
  std::size_t _total_components{};
  // Index of global minimum in Data vector. This minimum is never paired.
  bool _alive_components_verified{};

 public:
  /*!
          Call this function with a vector of one dimensional data to find extrema features in the
     data. The function runs once for, results can be retrieved with different persistent thresholds
     without further data processing.

          Input data vector is assumed to be of legal size and legal values.

          Use PrintResults, GetPairedExtrema or GetExtremaIndices to get results of the function.

          @param[in] InputData Vector of data to find features on, ordered according to its axis.
  */
  bool RunPersistence(const std::vector<double>& InputData) {
    // If a user runs this on an empty vector, then they should not get the results of the previous
    // run.
    if (InputData.empty()) {
      _data.clear();
      Init();
      return false;
    }

    _data = InputData;
    Init();

    CreateIndexValueVector();
    Watershed();
    SortPairedExtrema();
#ifndef NDEBUG
    VerifyAliveComponents();
#endif
    return true;
  }

  /*!
          Prints the contents of the TPairedExtrema vector.
          If called directly with a TPairedExtrema vector, the global minimum is not printed.

          @param[in] pairs	Vector of pairs to be printed.
  */
  static void PrintPairs(const std::vector<TPairedExtrema>& pairs) {
    for (const auto& p : pairs) {
      std::cout << "Persistence: " << p.Persistence << " minimum index: " << p.MinIndex
                << " maximum index: " << p.MaxIndex << '\n';
    }
  }

  /*!
          Prints the global minimum and all paired extrema whose persistence is greater or equal to
     threshold. By default, all pairs are printed. Supports Matlab indexing.

          @param[in] threshold		Threshold value for pair persistence.
          @param[in] matlabIndexing	Use Matlab indexing for printing.
  */
  void PrintResults(const double threshold = 0.0, const bool matlabIndexing = false) const {
    if (threshold < 0) {
      std::cout << "Error. Threshold value must be greater than or equal to 0\n";
    }
    if (threshold == 0 && !matlabIndexing) {
      PrintPairs(_poaired_extrema);
    } else {
      std::vector<TPairedExtrema> pairs;
      GetPairedExtrema(pairs, threshold, matlabIndexing);
      PrintPairs(pairs);
    }

    std::cout << "Global minimum value: " << GetGlobalMinimumValue()
              << " index: " << GetGlobalMinimumIndex(matlabIndexing) << '\n';
  }

  /*!
          Use this method to get the results of RunPersistence.
          Returned pairs are sorted according to persistence, from least to most persistent.

          @param[out]	pairs			Destination vector for PairedExtrema
          @param[in]	threshold		Minimal persistence value of returned features. All
     PairedExtrema with persistence equal to or above this value will be returned. If left to
     default, all PairedMaxima will be returned.

          @param[in] matlabIndexing	Set this to true to change all indices of features to
     Matlab's 1-indexing.
  */
  bool GetPairedExtrema(std::vector<TPairedExtrema>& pairs, const double threshold = 0,
                        std::int64_t offset = 0) const {
    // make sure the user does not use previous results that do not match the data
    pairs.clear();

    if (_poaired_extrema.empty() || threshold < 0.0) {
      return false;
    }

    const auto lower_bound = FilterByPersistence(threshold);

    if (lower_bound == _poaired_extrema.end()) {
      return false;
    }

    pairs.insert(pairs.begin(), lower_bound, _poaired_extrema.end());

    if (offset != 0) {
      for (auto& p : pairs) {
        p.MinIndex += offset;
        p.MaxIndex += offset;
      }
    }
    return true;
  }

  /*!
  Use this method to get two vectors with all indices of PairedExterma.
  Returns false if no paired features were found.
  Returned vectors have the same length.
  Overwrites any data contained in min, max vectors.

  @param[out] min				Vector of indices of paired local minima.
  @param[out]	max				Vector of indices of paired local maxima.
  @param[in]	threshold		Return only indices for pairs whose persistence is greater
  than or equal to threshold.
  @param[in]	matlabIndexing	Set this to true to change all indices to match Matlab's 1-indexing.
*/
  bool GetExtremaIndices(std::vector<std::int64_t>& min, std::vector<std::int64_t>& max,
                         const double threshold = 0, std::int64_t offset = 0) const {
    // before doing anything, make sure the user does not use old results
    min.clear();
    max.clear();

    if (_poaired_extrema.empty() || threshold < 0.0) {
      return false;
    }

    const auto lower_bound = FilterByPersistence(threshold);
    const auto size = static_cast<std::size_t>(std::distance(lower_bound, _poaired_extrema.end()));
    min.reserve(size);
    max.reserve(size);

    std::for_each(lower_bound, _poaired_extrema.end(), [&](const auto& p) {
      min.push_back(p.MinIndex + offset);
      max.push_back(p.MaxIndex + offset);
    });
    return true;
  }
  /*!
          Returns the index of the global minimum.
          The global minimum does not get paired and is not returned
          via GetPairedExtrema and GetExtremaIndices.
  */
  [[nodiscard]] std::int64_t GetGlobalMinimumIndex(std::int64_t offset = 0) const noexcept {
    if (_components.empty()) {
      return -1;
    }

    assert(_components.front().Alive);
    return _components.front().MinIndex + offset;
  }

  /*!
          Returns the value of the global minimum.
          The global minimum does not get paired and is not returned
          via GetPairedExtrema and GetExtremaIndices.
  */
  [[nodiscard]] double GetGlobalMinimumValue() const noexcept {
    if (_components.empty()) {
      return 0;
    }

    assert(_components.front().Alive);
    return _components.front().MinValue;
  }
  /*!
          Runs basic sanity checks on results of RunPersistence:
          - Number of unique minima = number of unique maxima - 1 (Morse property)
          - All returned indices are unique (no index is returned as two extrema)
          - Global minimum is within domain indices or at default value
          - Global minimum is not returned as any other extrema.
          - Global minimum is not paired.

          Returns true if run results pass these sanity checks.
  */
  bool VerifyResults() {
    std::vector<std::int64_t> min{};
    std::vector<std::int64_t> max{};
    std::vector<std::int64_t> combinedIndices{};

    GetExtremaIndices(min, max);

    const auto globalMinIdx = GetGlobalMinimumIndex();

    std::sort(min.begin(), min.end());
    std::sort(max.begin(), max.end());
    combinedIndices.reserve(min.size() + max.size());
    std::set_union(min.begin(), min.end(), max.begin(), max.end(),
                   std::inserter(combinedIndices, combinedIndices.begin()));

    // check the combined unique indices are equal to size of min and max
    if (combinedIndices.size() != (min.size() + max.size()) ||
        std::binary_search(combinedIndices.begin(), combinedIndices.end(), globalMinIdx)) {
      return false;
    }

    if ((globalMinIdx > (int)_data.size() - 1) || (globalMinIdx < -1)) {
      return false;
    }
    if (globalMinIdx == -1 && !min.empty()) {
      return false;
    }

    const auto minUniqueEnd = std::unique(min.begin(), min.end());
    const auto maxUniqueEnd = std::unique(max.begin(), max.end());

    if (minUniqueEnd != min.end() || maxUniqueEnd != max.end() ||
        (minUniqueEnd - min.begin()) != (maxUniqueEnd - max.begin())) {
      return false;
    }

    return true;
  }

 private:
  /*!
          Merges two components by doing the following:

          - Destroys component with smaller hub (sets Alive=false).
          - Updates surviving component's edges to span the destroyed component's region.
          - Updates the destroyed component's edge vertex colors to the survivor's color in
     Colors[].

          @param[in] firstIdx,secondIdx	Indices of components to be merged. Their order does not
     matter.
  */
  void MergeComponents(const std::int64_t firstIdx, const std::int64_t secondIdx) {
    std::int64_t survivorIdx{};
    std::int64_t destroyedIdx{};
    // survivor - component whose hub is bigger
    if (_components[firstIdx].MinValue < _components[secondIdx].MinValue) {
      survivorIdx = firstIdx;
      destroyedIdx = secondIdx;
    } else if (_components[firstIdx].MinValue > _components[secondIdx].MinValue) {
      survivorIdx = secondIdx;
      destroyedIdx = firstIdx;
    } else if (firstIdx < secondIdx) {
      // Both components min values are equal, destroy component on
      // the right This is done to fit with the left-to-right total
      // ordering of the values
      survivorIdx = firstIdx;
      destroyedIdx = secondIdx;
    } else {
      survivorIdx = secondIdx;
      destroyedIdx = firstIdx;
    }

    // survivor and destroyed are decided, now destroy!
    _components[destroyedIdx].Alive = false;

    // Update the color of the edges of the destroyed component to the color of the surviving
    // component.
    _colors[_components[destroyedIdx].RightEdgeIndex] = survivorIdx;
    _colors[_components[destroyedIdx].LeftEdgeIndex] = survivorIdx;

    // Update the relevant edge index of surviving component, such that it contains the destroyed
    // component's region.
    if (_components[survivorIdx].MinIndex > _components[destroyedIdx].MinIndex) {
      // destroyed index to the left of survivor, update left edge
      _components[survivorIdx].LeftEdgeIndex = _components[destroyedIdx].LeftEdgeIndex;
    } else {
      _components[survivorIdx].RightEdgeIndex = _components[destroyedIdx].RightEdgeIndex;
    }
  }

  /*!
          Creates a new PairedExtrema from the two indices, and adds it to PairedFeatures.

          @param[in] firstIdx, secondIdx Indices of vertices to be paired. Order does not matter.
  */
  void CreatePairedExtrema(const std::int64_t firstIdx, const std::int64_t secondIdx) {
    TPairedExtrema pair{};

    // There might be a potential bug here, todo (we're checking data, not sorted data)
    // example case: 1 1 1 1 1 1 -5 might remove if after else
    if (_data[firstIdx] > _data[secondIdx]) {
      pair.MaxIndex = firstIdx;
      pair.MinIndex = secondIdx;
    } else if (_data[secondIdx] > _data[firstIdx]) {
      pair.MaxIndex = secondIdx;
      pair.MinIndex = firstIdx;
    }
    // both values are equal, choose the left one as the min
    else if (firstIdx < secondIdx) {
      pair.MinIndex = firstIdx;
      pair.MaxIndex = secondIdx;
    } else {
      pair.MinIndex = secondIdx;
      pair.MaxIndex = firstIdx;
    }

    pair.Persistence = _data[pair.MaxIndex] - _data[pair.MinIndex];

    assert(pair.Persistence >= 0);
    if (_poaired_extrema.capacity() == _poaired_extrema.size()) {
      _poaired_extrema.reserve(_poaired_extrema.size() * 2 + 1);
    }

    _poaired_extrema.push_back(pair);
  }

  // Changing the alignment of the next Doxygen comment block breaks its formatting.

  /*! Creates a new component at a local minimum.

  Neighboring vertices are assumed to have no color.
  - Adds a new component to the components vector,
  - Initializes its edges and minimum index to minIdx.
  - Updates Colors[minIdx] to the component's color.

  @param[in]	minIdx Index of a local minimum.
  */
  void CreateComponent(const std::int64_t minIdx) {
    assert(minIdx >= 0);
    const auto i = static_cast<std::size_t>(minIdx);
    TComponent comp{i, i, i, _data[i], true};

    // place at the end of component vector and get the current size
    if (_components.capacity() <= _total_components) {
      _components.reserve(2 * _total_components + 1);
    }

    _components.push_back(comp);
    _colors[minIdx] = static_cast<std::int64_t>(_total_components++);
  }

  /*!
          Extends the component's region by one vertex:

          - Updates the matching component's edge to dataIdx..
          - updates Colors[dataIdx] to the component's color.

          @param[in]	componentIdx	Index of component (the value of a neighboring vertex in
     Colors[]).
          @param[in] 	dataIdx			Index of vertex which the component is extended to.
  */
  void ExtendComponent(const std::int64_t componentIdx, const std::int64_t dataIdx) {
    assert(_components[componentIdx].Alive);

    // extend to the left
    if (dataIdx + 1 == _components[componentIdx].LeftEdgeIndex) {
      _components[componentIdx].LeftEdgeIndex = dataIdx;
    } else if (dataIdx - 1 == _components[componentIdx].RightEdgeIndex) {
      // extend to the right
      _components[componentIdx].RightEdgeIndex = dataIdx;
    } else {
      throw std::runtime_error("ExtendComponent: index mismatch. Data index: " +
                               std::to_string(dataIdx));
    }

    _colors[dataIdx] = componentIdx;
  }

  /*!
          Initializes main data structures used in class:
          - Sets Colors[] to NO_COLOR
          - Reserves memory for Components and PairedExtrema

          Note: SortedData is should be created before, separately, using CreateIndexValueVector()
  */
  void Init() {
    _sorted_data.clear();
    _sorted_data.reserve(_data.size());

    _colors.clear();
    _colors.resize(_data.size());
    std::fill(_colors.begin(), _colors.end(), NO_COLOR);

    // starting reserved size >= 1 at least
    const auto vectorSize = (_data.size() / RESIZE_FACTOR) + 1;

    _components.clear();
    _components.reserve(vectorSize);

    _poaired_extrema.clear();
    _poaired_extrema.reserve(vectorSize);

    _total_components = 0;
    _alive_components_verified = false;
  }

  /*!
          Creates SortedData vector.
          Assumes Data is already set.
  */
  void CreateIndexValueVector() {
    if (_data.empty()) {
      return;
    }

    for (std::size_t i = 0; i != _data.size(); ++i) {
      // this is going to make problems
      _sorted_data.emplace_back(TIdxAndData{static_cast<std::int64_t>(i), _data[i]});
    }

    std::sort(_sorted_data.begin(), _sorted_data.end());
  }

  /*!
          Main algorithm - all of the work happen here.

          Use only after calling CreateIndexValueVector and Init functions.

          Iterates over each vertex in the graph according to their ordered values:
          - Creates a segment for each local minima
          - Extends a segment is data has only one neighboring component
          - Merges segments and creates new PairedExtrema when a vertex has two neighboring
     components.
  */
  void Watershed() {
    if (_sorted_data.size() == 1) {
      CreateComponent(0);
      return;
    }

    for (auto& p : _sorted_data) {
      assert(p.Idx >= 0);
      const auto i0 = static_cast<std::size_t>(p.Idx - 1);  // this can overflow but it is fine
      const auto i = p.Idx;
      const auto i1 = static_cast<std::size_t>(p.Idx + 1);
      const auto ii = p.Idx;

      // left most vertex - no left neighbor
      // two options - either local minimum, or extend component
      if (i == 0) {
        if (_colors[i1] == NO_COLOR) {
          CreateComponent(i);
        } else {
          ExtendComponent(_colors[i1], i);  // in this case, local max as well
        }
        continue;
      }

      // right most vertex - look only to the left
      if (i == _colors.size() - 1) {
        if (_colors[i0] == NO_COLOR) {
          CreateComponent(i);
        } else {
          ExtendComponent(_colors[i0], i);
        }
        continue;
      }

      // look left and right
      if (_colors[i0] == NO_COLOR && _colors[i1] == NO_COLOR) {
        // local minimum - create new component
        CreateComponent(i);
      } else if (_colors[i0] != NO_COLOR && _colors[i1] == NO_COLOR) {
        // single neighbor on the left - extend
        ExtendComponent(_colors[i0], i);
      } else if (_colors[i0] == NO_COLOR && _colors[i1] != NO_COLOR) {
        // single component on the right - extend
        ExtendComponent(_colors[i1], i);
      } else if (_colors[i0] != NO_COLOR && _colors[i1] != NO_COLOR) {
        // local maximum - merge components
        const auto leftComp = _colors[i0];
        const auto rightComp = _colors[i1];

        // choose component with smaller hub destroyed component
        if (_components[rightComp].MinValue < _components[leftComp].MinValue) {
          // left component has smaller hub
          CreatePairedExtrema(static_cast<std::int64_t>(_components[leftComp].MinIndex), i);
        } else {
          // either right component has smaller hub, or hubs are equal - destroy right component.
          CreatePairedExtrema(static_cast<std::int64_t>(_components[rightComp].MinIndex), i);
        }

        MergeComponents(leftComp, rightComp);
        _colors[i] = _colors[i0];  // color should be correct at both sides at this point
      }
    }
  }

  /*!
          Sorts the PairedExtrema list according to the persistence of the features.
          Orders features with equal persistence according the the index of their minima.
  */
  void SortPairedExtrema() { std::sort(_poaired_extrema.begin(), _poaired_extrema.end()); }

  /*!
          Returns an iterator to the first element in PairedExtrema whose persistence is bigger or
     equal to threshold. If threshold is set to 0, returns an iterator to the first object in
     PairedExtrema.

          @param[in]	threshold	Minimum persistence of features to be returned.
  */
  [[nodiscard]] std::vector<TPairedExtrema>::const_iterator FilterByPersistence(
      const double threshold = 0) const {
    if (threshold <= 0) {
      return _poaired_extrema.begin();
    }

    const TPairedExtrema searchPair{0, 0, threshold};
    return std::lower_bound(_poaired_extrema.begin(), _poaired_extrema.end(), searchPair);
  }
  /*!
          Runs at the end of RunPersistence, after Watershed.
          Algorithm results should be as followed:
          - All but one components should not be Alive.
          - The Alive component contains the global minimum.
          - The Alive component should be the first component in the Component vector
  */
  void VerifyAliveComponents() const {
    if (_components.empty()) {
      return;
    }
    // verify that the Alive component is component #0 (contains global minimum by definition)
    if (!_components.front().Alive) {
      throw std::runtime_error(
          "Error. Component 0 is not Alive, assumed to contain global minimum");
    }

    for (std::size_t i = 1; i < _components.size(); ++i) {
      if (_components[i].Alive) {
        throw std::runtime_error("Error. Found more than one alive component");
      }
    }
  }
};

}  // namespace p1d
