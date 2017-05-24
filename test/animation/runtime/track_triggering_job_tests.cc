//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) 2015 Guillaume Blanc                                         //
//                                                                            //
// Permission is hereby granted, free of charge, to any person obtaining a    //
// copy of this software and associated documentation files (the "Software"), //
// to deal in the Software without restriction, including without limitation  //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,   //
// and/or sell copies of the Software, and to permit persons to whom the      //
// Software is furnished to do so, subject to the following conditions:       //
//                                                                            //
// The above copyright notice and this permission notice shall be included in //
// all copies or substantial portions of the Software.                        //
//                                                                            //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR //
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   //
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    //
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER //
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    //
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        //
// DEALINGS IN THE SOFTWARE.                                                  //
//                                                                            //
//----------------------------------------------------------------------------//

#include "ozz/animation/runtime/track_triggering_job.h"

#include "gtest/gtest.h"

#include "ozz/base/maths/gtest_math_helper.h"
#include "ozz/base/memory/allocator.h"

#include "ozz/animation/offline/raw_track.h"
#include "ozz/animation/offline/track_builder.h"

#include "ozz/animation/runtime/track.h"

using ozz::animation::FloatTrack;
using ozz::animation::FloatTrackTriggeringJob;
using ozz::animation::offline::RawFloatTrack;
using ozz::animation::offline::TrackBuilder;
using ozz::animation::offline::RawTrackInterpolation;

TEST(JobValidity, FloatTrackTriggeringJob) {
  /*
  // Instantiates a builder objects with default parameters.
  TrackBuilder builder;

  // Building default RawFloatTrack succeeds.
  RawFloatTrack raw_float_track;
  EXPECT_TRUE(raw_float_track.Validate());

  // Builds track
  FloatTrack* track = builder(raw_float_track);
  EXPECT_TRUE(track != NULL);

  {  // Empty/default job
    FloatTrackSamplingJob job;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }

  {  // Invalid output
    FloatTrackSamplingJob job;
    job.track = track;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }

  {  // Invalid track.
    FloatTrackSamplingJob job;
    float result;
    job.result = &result;
    EXPECT_FALSE(job.Validate());
    EXPECT_FALSE(job.Run());
  }

  {  // Valid
    FloatTrackSamplingJob job;
    job.track = track;
    float result;
    job.result = &result;
    EXPECT_TRUE(job.Validate());
    EXPECT_TRUE(job.Run());
  }
  ozz::memory::default_allocator()->Delete(track);
*/
}

TEST(NoRange, TrackEdgeTriggerJob) {
  TrackBuilder builder;
  FloatTrackTriggeringJob::Edge edges_buffer[8];
  ozz::Range<FloatTrackTriggeringJob::Edge> edges(edges_buffer);

  ozz::animation::offline::RawFloatTrack raw_track;

  // Keyframe values oscillate in range [0,2].
  const ozz::animation::offline::RawFloatTrack::Keyframe key0 = {
      RawTrackInterpolation::kStep, 0.f, 0.f};
  raw_track.keyframes.push_back(key0);
  const ozz::animation::offline::RawFloatTrack::Keyframe key1 = {
      RawTrackInterpolation::kStep, .5f, 2.f};
  raw_track.keyframes.push_back(key1);
  const ozz::animation::offline::RawFloatTrack::Keyframe key2 = {
      RawTrackInterpolation::kStep, 1.f, 0.f};
  raw_track.keyframes.push_back(key2);

  // Builds track
  ozz::animation::FloatTrack* track = builder(raw_track);
  ASSERT_TRUE(track != NULL);

  FloatTrackTriggeringJob job;
  job.track = track;
  job.edges = &edges;
  int num_edges = 0;
  job.num_edges = &num_edges;
  job.threshold = 1.f;

  {  // Forward [0., 0.[
    job.from = 0.f;
    job.to = 0.f;
    EXPECT_TRUE(job.Run());

    ASSERT_EQ(num_edges, 0);
  }

  {  // Forward [.1, .1]
    job.from = .1f;
    job.to = .1f;
    EXPECT_TRUE(job.Run());

    ASSERT_EQ(num_edges, 0);
  }

  {  // Forward [.5, .5[
    job.from = .5f;
    job.to = .5f;
    EXPECT_TRUE(job.Run());

    ASSERT_EQ(num_edges, 0);
  }

  {  // Forward [1., 1.]
    job.from = 1.f;
    job.to = 1.f;
    EXPECT_TRUE(job.Run());

    ASSERT_EQ(num_edges, 0);
  }

  ozz::memory::default_allocator()->Delete(track);
}

void TestEdgesIntegrity(const ozz::animation::FloatTrack& _track) {
  FloatTrackTriggeringJob::Edge edges_buffer[128];
  ozz::Range<FloatTrackTriggeringJob::Edge> edges(edges_buffer);

  FloatTrackTriggeringJob job;
  job.track = &_track;
  job.edges = &edges;
  int num_edges = 0;
  job.num_edges = &num_edges;
  job.threshold = 1.f;

  float time = 0.f;
  for (size_t i = 0; i < 100000; ++i) {
    // Finds new evaluation range
    const float kMaxLoops = 3.f;
    const float rand_range = 1.f - 2 * static_cast<float>(rand()) / RAND_MAX;

    job.from = time;
    time += kMaxLoops * rand_range;
    if (time < 0) {
      time = -time;
    }
    job.to = time;
    EXPECT_TRUE(job.Run());

    // Check edges.
    bool rising = false;
    bool init = false;
    for (size_t e = 0; e < num_edges; ++e) {
      if (!init) {
        rising = edges[e].rising;
        init = true;
      } else {
        if (rising == edges[e].rising) {
          break;
        }
        // Successive edges should always be opposed, whichever direction the
        // time is going.
        EXPECT_TRUE(rising != edges[e].rising);
        rising = edges[e].rising;
      }
    }
  }
}

TEST(SquareStep, TrackEdgeTriggerJob) {
  TrackBuilder builder;
  FloatTrackTriggeringJob::Edge edges_buffer[8];
  ozz::Range<FloatTrackTriggeringJob::Edge> edges(edges_buffer);

  {  // Rising edge at t = 0.5
    ozz::animation::offline::RawFloatTrack raw_track;

    // Keyframe values oscillate in range [0,2].
    const ozz::animation::offline::RawFloatTrack::Keyframe key0 = {
        RawTrackInterpolation::kStep, 0.f, 0.f};
    raw_track.keyframes.push_back(key0);
    const ozz::animation::offline::RawFloatTrack::Keyframe key1 = {
        RawTrackInterpolation::kStep, .5f, 2.f};
    raw_track.keyframes.push_back(key1);
    const ozz::animation::offline::RawFloatTrack::Keyframe key2 = {
        RawTrackInterpolation::kStep, 1.f, 0.f};
    raw_track.keyframes.push_back(key2);

    // Builds track
    ozz::animation::FloatTrack* track = builder(raw_track);
    ASSERT_TRUE(track != NULL);

    FloatTrackTriggeringJob job;
    job.track = track;
    job.edges = &edges;
    int num_edges = 0;
    job.num_edges = &num_edges;
    job.threshold = 1.f;

    // Forward

    {  // Forward [0, .99[, 1 is excluded
      job.from = 0.f;
      job.to = .99f;

      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time,
                      .5f);  // "Step" edges uses exact time comparison.
      EXPECT_EQ(edges[0].rising, true);
    }

    {  // Forward [0, 1], 1 is included
      job.from = 0.f;
      job.to = 1.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, .5f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, 1.f);
      EXPECT_EQ(edges[1].rising, false);
    }

    {  // Forward [0, .5[
      job.from = 0.f;
      job.to = .5f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 0);
    }

    {  // Forward [.1, .5[
      job.from = .1f;
      job.to = .5f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 0);
    }

    {  // Forward [.5, .9[
      job.from = .5;
      job.to = .9f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, .5f);
      EXPECT_EQ(edges[0].rising, true);
    }

    {  // Forward [.6, .9[
      job.from = .6f;
      job.to = .9f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 0);
    }

    {  // Forward [.9, 1.], 1 is included
      job.from = .9f;
      job.to = 1.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, 1.f);
      EXPECT_EQ(edges[0].rising, false);
    }

    {  // Forward [.5, 1.], 1 is included
      job.from = .5f;
      job.to = 1.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, .5f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, 1.f);
      EXPECT_EQ(edges[1].rising, false);
    }

    {  // Forward loop [0., 2.],
      job.from = 0.f;
      job.to = 2.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 4);

      EXPECT_FLOAT_EQ(edges[0].time, .5f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, 1.f);
      EXPECT_EQ(edges[1].rising, false);

      EXPECT_FLOAT_EQ(edges[2].time, 1.5f);
      EXPECT_EQ(edges[2].rising, true);

      EXPECT_EQ(edges[3].time, 2.f);
      EXPECT_EQ(edges[3].rising, false);
    }

    {  // Forward loop [0., 1.5[,
      job.from = 0.f;
      job.to = 1.5f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, .5f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, 1.f);
      EXPECT_EQ(edges[1].rising, false);
    }

    {  // Forward loop [0., 1.],
      job.from = 0.f;
      job.to = 1.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, .5f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, 1.f);
      EXPECT_EQ(edges[1].rising, false);
    }

    {  // Forward loop ]1., 2.],
      job.from = 1.f;
      job.to = 2.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, 1.5f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, 2.f);
      EXPECT_EQ(edges[1].rising, false);
    }

    {  // Forward loop [1., 1.6[,
      job.from = 1.f;
      job.to = 1.6f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, 1.5f);
      EXPECT_EQ(edges[0].rising, true);
    }

    {  // Forward loop [.9, 1.6[,
      job.from = .9f;
      job.to = 1.6f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, 1.f);
      EXPECT_EQ(edges[0].rising, false);

      EXPECT_FLOAT_EQ(edges[1].time, 1.5f);
      EXPECT_EQ(edges[1].rising, true);
    }

    {  // Forward out of bound[1.5, 1.9[
      job.from = 1.5f;
      job.to = 1.9f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, 1.5f);
      EXPECT_EQ(edges[0].rising, true);
    }

    {  // Forward out of bound[1.5, 3.[
      job.from = 1.5f;
      job.to = 3.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 4);

      EXPECT_FLOAT_EQ(edges[0].time, 1.5f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, 2.f);
      EXPECT_EQ(edges[1].rising, false);

      EXPECT_FLOAT_EQ(edges[2].time, 2.5f);
      EXPECT_EQ(edges[2].rising, true);

      EXPECT_EQ(edges[3].time, 3.f);
      EXPECT_EQ(edges[3].rising, false);
    }

    // Backward

    {  // Backward [1, .01], 0 is excluded
      job.from = 1.f;
      job.to = .01f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, 1.f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, .5f);
      EXPECT_EQ(edges[1].rising, false);
    }

    {  // Backward [1, 0], 0 is included
      job.from = 1.f;
      job.to = 0.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, 1.f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, .5f);
      EXPECT_EQ(edges[1].rising, false);
    }

    {  // Backward ].5, 0]
      job.from = .5f;
      job.to = 0.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 0);
    }

    {  // Backward ].9, .5]
      job.from = .9f;
      job.to = .5f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, .5f);
      EXPECT_EQ(edges[0].rising, false);
    }

    {  // Backward [1., .5]
      job.from = 1.f;
      job.to = .5f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, 1.f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, .5f);
      EXPECT_EQ(edges[1].rising, false);
    }

    {  // Backward ].4, .1]
      job.from = .4f;
      job.to = .1f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 0);
    }

    {  // Backward loop [2., 0.]
      job.from = 2.f;
      job.to = 0.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 4);

      EXPECT_FLOAT_EQ(edges[0].time, 2.f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, 1.5f);
      EXPECT_EQ(edges[1].rising, false);

      EXPECT_FLOAT_EQ(edges[2].time, 1.f);
      EXPECT_EQ(edges[2].rising, true);

      EXPECT_EQ(edges[3].time, .5f);
      EXPECT_EQ(edges[3].rising, false);
    }

    {  // Backward loop ]1.5, 0]
      job.from = 1.5f;
      job.to = 0.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, 1.f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, .5f);
      EXPECT_EQ(edges[1].rising, false);
    }

    {  // Backward loop [2., 1.]
      job.from = 1.f;
      job.to = 0.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, 1.f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, .5f);
      EXPECT_EQ(edges[1].rising, false);
    }

    {  // Backward loop [1., 0.]
      job.from = 1.f;
      job.to = 0.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, 1.f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, .5f);
      EXPECT_EQ(edges[1].rising, false);
    }

    {  // Backward loop ]1.5, 1.]
      job.from = 1.5f;
      job.to = 1.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 0);
    }

    {  // Backward loop ]1.6, .9]
      job.from = 1.6f;
      job.to = .9f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, 1.5f);
      EXPECT_EQ(edges[0].rising, false);

      EXPECT_FLOAT_EQ(edges[1].time, 1.f);
      EXPECT_EQ(edges[1].rising, true);
    }

    {  // Backward out of bound ]1.5, 1.1]
      job.from = 1.5f;
      job.to = 1.1f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 0);
    }

    {  // Backward out of bound [3., 1.5]
      job.from = 3.f;
      job.to = 1.5f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 4);

      EXPECT_FLOAT_EQ(edges[0].time, 3.f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, 2.5f);
      EXPECT_EQ(edges[1].rising, false);

      EXPECT_FLOAT_EQ(edges[2].time, 2.f);
      EXPECT_EQ(edges[2].rising, true);

      EXPECT_EQ(edges[3].time, 1.5f);
      EXPECT_EQ(edges[3].rising, false);
    }

    // Rewind loop

    {  // [0., 2.]
      job.from = 0.f;
      job.to = 2.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 4);

      EXPECT_FLOAT_EQ(edges[0].time, .5f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, 1.f);
      EXPECT_EQ(edges[1].rising, false);

      EXPECT_FLOAT_EQ(edges[2].time, 1.5f);
      EXPECT_EQ(edges[2].rising, true);

      EXPECT_EQ(edges[3].time, 2.f);
      EXPECT_EQ(edges[3].rising, false);
    }

    {  // [2., 0.]
      job.from = 2.f;
      job.to = 0.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 4);

      EXPECT_FLOAT_EQ(edges[0].time, 2.f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, 1.5f);
      EXPECT_EQ(edges[1].rising, false);

      EXPECT_FLOAT_EQ(edges[2].time, 1.f);
      EXPECT_EQ(edges[2].rising, true);

      EXPECT_EQ(edges[3].time, .5f);
      EXPECT_EQ(edges[3].rising, false);
    }

    {  // [.7, 1.5[
      job.from = .7f;
      job.to = 1.5f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, 1.f);
      EXPECT_EQ(edges[0].rising, false);
    }

    {  // ]1.5, .7]
      job.from = 1.5f;
      job.to = .7f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, 1.f);
      EXPECT_EQ(edges[0].rising, true);
    }

    // Negative times

    {  // [-1, 0[
      job.from = -1.f;
      job.to = 0.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, -.5f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, 0.f);
      EXPECT_EQ(edges[1].rising, false);
    }

    {  // [-1.5, 0.75[
      job.from = -1.5f;
      job.to = .5f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 4);

      EXPECT_FLOAT_EQ(edges[0].time, -1.5f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, -1.f);
      EXPECT_EQ(edges[1].rising, false);

      EXPECT_FLOAT_EQ(edges[2].time, -.5f);
      EXPECT_EQ(edges[2].rising, true);

      EXPECT_EQ(edges[3].time, 0.f);
      EXPECT_EQ(edges[3].rising, false);
    }

    // Negative backward times

    {  // [0, -1]
      job.from = 0.f;
      job.to = -1.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, 0.f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, -.5f);
      EXPECT_EQ(edges[1].rising, false);
    }

    {  // ]0.5, -1.5]
      job.from = .5f;
      job.to = -1.5f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 4);

      EXPECT_FLOAT_EQ(edges[0].time, 0.f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, -.5f);
      EXPECT_EQ(edges[1].rising, false);

      EXPECT_FLOAT_EQ(edges[2].time, -1.f);
      EXPECT_EQ(edges[2].rising, true);

      EXPECT_EQ(edges[3].time, -1.5f);
      EXPECT_EQ(edges[3].rising, false);
    }

    {  // ]0.6, -1.4]
      job.from = .6f;
      job.to = -1.4f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 4);

      EXPECT_FLOAT_EQ(edges[0].time, .5f);
      EXPECT_EQ(edges[0].rising, false);

      EXPECT_FLOAT_EQ(edges[1].time, 0.f);
      EXPECT_EQ(edges[1].rising, true);

      EXPECT_FLOAT_EQ(edges[2].time, -.5f);
      EXPECT_EQ(edges[2].rising, false);

      EXPECT_EQ(edges[3].time, -1.f);
      EXPECT_EQ(edges[3].rising, true);
    }

    TestEdgesIntegrity(*track);

    ozz::memory::default_allocator()->Delete(track);
  }

  {  // Rising edge at t = 0.6, no falling edge at end
    ozz::animation::offline::RawFloatTrack raw_track;

    // Keyframe values oscillate in range [0,2].
    const ozz::animation::offline::RawFloatTrack::Keyframe key0 = {
        RawTrackInterpolation::kStep, 0.f, 0.f};
    raw_track.keyframes.push_back(key0);
    const ozz::animation::offline::RawFloatTrack::Keyframe key1 = {
        RawTrackInterpolation::kStep, .6f, 2.f};
    raw_track.keyframes.push_back(key1);

    // Builds track
    ozz::animation::FloatTrack* track = builder(raw_track);
    ASSERT_TRUE(track != NULL);

    FloatTrackTriggeringJob job;
    job.track = track;
    job.edges = &edges;
    int num_edges = 0;
    job.num_edges = &num_edges;
    job.threshold = 1.f;

    {  // Forward [0, .99[, 1 is excluded
      job.from = 0.f;
      job.to = .99f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time,
                      .0f);  // "Step" edges uses exact time comparison.
      EXPECT_EQ(edges[0].rising, false);

      EXPECT_FLOAT_EQ(edges[1].time, .6f);
      EXPECT_EQ(edges[1].rising, true);
    }

    {  // Forward [0, 1], 1 is included
      job.from = 0.f;
      job.to = 1.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, 0.f);
      EXPECT_EQ(edges[0].rising, false);

      EXPECT_FLOAT_EQ(edges[1].time, .6f);
      EXPECT_EQ(edges[1].rising, true);
    }

    {  // Forward [1, 2], 1 should be included, as it wasn't included with range
       // [0, 1]
      job.from = 1.f;
      job.to = 2.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, 1.f);
      EXPECT_EQ(edges[0].rising, false);

      EXPECT_FLOAT_EQ(edges[1].time, 1.6f);
      EXPECT_EQ(edges[1].rising, true);
    }

    {  // Forward [0, .6[
      job.from = 0.f;
      job.to = .6f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, 0.f);
      EXPECT_EQ(edges[0].rising, false);
    }

    {  // Forward [.1, .6[
      job.from = .1f;
      job.to = .6f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 0);
    }

    {  // Forward [.6, .9[
      job.from = .6f;
      job.to = .9f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, .6f);
      EXPECT_EQ(edges[0].rising, true);
    }

    {  // Forward [.7, .9[
      job.from = .7f;
      job.to = .9f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 0);
    }

    {  // Forward [.9, 1.], 1 is included
      job.from = .9f;
      job.to = 1.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 0);
    }

    {  // Forward [.6, 1.], 1 is included
      job.from = .6f;
      job.to = 1.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, .6f);
      EXPECT_EQ(edges[0].rising, true);
    }

    {  // Forward loop [0., 2.],
      job.from = 0.f;
      job.to = 2.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 4);

      EXPECT_FLOAT_EQ(edges[0].time, 0.f);
      EXPECT_EQ(edges[0].rising, false);

      EXPECT_FLOAT_EQ(edges[1].time, .6f);
      EXPECT_EQ(edges[1].rising, true);

      EXPECT_FLOAT_EQ(edges[2].time, 1.f);  // Falling edge created by the loop.
      EXPECT_EQ(edges[2].rising, false);

      EXPECT_EQ(edges[3].time, 1.6f);
      EXPECT_EQ(edges[3].rising, true);
    }

    {  // Forward loop [0., 1.6[,
      job.from = 0.f;
      job.to = 1.6f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 3);

      EXPECT_FLOAT_EQ(edges[0].time, 0.f);
      EXPECT_EQ(edges[0].rising, false);

      EXPECT_FLOAT_EQ(edges[1].time, .6f);
      EXPECT_EQ(edges[1].rising, true);

      EXPECT_FLOAT_EQ(edges[2].time, 1.f);
      EXPECT_EQ(edges[2].rising, false);
    }

    {  // Forward out of bound[1.6, 1.9[
      job.from = 1.6f;
      job.to = 1.9f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, 1.6f);
      EXPECT_EQ(edges[0].rising, true);
    }

    {  // Forward out of bound[1.6, 3.]
      job.from = 1.6f;
      job.to = 3.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 3);

      EXPECT_FLOAT_EQ(edges[0].time, 1.6f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, 2.f);
      EXPECT_EQ(edges[1].rising, false);

      EXPECT_FLOAT_EQ(edges[2].time, 2.6f);
      EXPECT_EQ(edges[2].rising, true);
    }

    {  // Forward loop [1., 1.7[,
      job.from = 1.f;
      job.to = 1.7f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      // Creates a falling edge because it's like a loop.
      EXPECT_FLOAT_EQ(edges[0].time, 1.f);
      EXPECT_EQ(edges[0].rising, false);

      EXPECT_FLOAT_EQ(edges[1].time, 1.6f);
      EXPECT_EQ(edges[1].rising, true);
    }

    {  // Forward loop [.9, 1.7[,
      job.from = .9f;
      job.to = 1.7f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, 1.f);
      EXPECT_EQ(edges[0].rising, false);

      EXPECT_FLOAT_EQ(edges[1].time, 1.6f);
      EXPECT_EQ(edges[1].rising, true);
    }

    // Backward

    {  // Backward [1, .01], 0 is excluded
      job.from = 1.f;
      job.to = .01f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, .6f);
      EXPECT_EQ(edges[0].rising, false);
    }

    {  // Backward [1, 0], 0 is included
      job.from = 1.f;
      job.to = 0.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, .6f);
      EXPECT_EQ(edges[0].rising, false);

      EXPECT_FLOAT_EQ(edges[1].time, 0.f);
      EXPECT_EQ(edges[1].rising, true);
    }

    {  // Backward [2, 1],
      job.from = 2.f;
      job.to = 1.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, 1.6f);
      EXPECT_EQ(edges[0].rising, false);

      EXPECT_FLOAT_EQ(edges[1].time, 1.f);
      EXPECT_EQ(edges[1].rising, true);
    }

    {  // Backward ].6, 0]
      job.from = .6f;
      job.to = 0.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, 0.f);
      EXPECT_EQ(edges[0].rising, true);
    }

    {  // Backward ].9, .6]
      job.from = .9f;
      job.to = .6f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, .6f);
      EXPECT_EQ(edges[0].rising, false);
    }

    {  // Backward [1., .6]
      job.from = 1.f;
      job.to = .6f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, .6f);
      EXPECT_EQ(edges[0].rising, false);
    }

    {  // Backward ].4, .1]
      job.from = .4f;
      job.to = .1f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 0);
    }

    {  // Backward loop [2., 0.]
      job.from = 2.f;
      job.to = 0.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 4);

      EXPECT_FLOAT_EQ(edges[0].time, 1.6f);
      EXPECT_EQ(edges[0].rising, false);

      EXPECT_FLOAT_EQ(edges[1].time, 1.f);
      EXPECT_EQ(edges[1].rising, true);

      EXPECT_FLOAT_EQ(edges[2].time, .6f);
      EXPECT_EQ(edges[2].rising, false);

      EXPECT_EQ(edges[3].time, 0.f);
      EXPECT_EQ(edges[3].rising, true);
    }

    {  // Backward loop ]1.6, 0]
      job.from = 1.6f;
      job.to = 0.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 3);

      EXPECT_FLOAT_EQ(edges[0].time, 1.f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, .6f);
      EXPECT_EQ(edges[1].rising, false);

      EXPECT_FLOAT_EQ(edges[2].time, 0.f);
      EXPECT_EQ(edges[2].rising, true);
    }

    {  // Backward loop ]1.6, 1.]
      job.from = 1.6f;
      job.to = 1.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, 1.f);
      EXPECT_EQ(edges[0].rising, true);
    }

    {  // Backward loop ]1.7, .9]
      job.from = 1.7f;
      job.to = .9f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, 1.6f);
      EXPECT_EQ(edges[0].rising, false);

      EXPECT_FLOAT_EQ(edges[1].time, 1.f);
      EXPECT_EQ(edges[1].rising, true);
    }

    {  // Backward out of bound [3., 1.6]
      job.from = 3.f;
      job.to = 1.6f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 3);

      EXPECT_FLOAT_EQ(edges[0].time, 2.6f);
      EXPECT_EQ(edges[0].rising, false);

      EXPECT_FLOAT_EQ(edges[1].time, 2.f);
      EXPECT_EQ(edges[1].rising, true);

      EXPECT_FLOAT_EQ(edges[2].time, 1.6f);
      EXPECT_EQ(edges[2].rising, false);
    }

    // rewind

    {  // [0., 2.]
      job.from = 0.f;
      job.to = 2.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 4);

      EXPECT_FLOAT_EQ(edges[0].time, 0.f);
      EXPECT_EQ(edges[0].rising, false);

      EXPECT_FLOAT_EQ(edges[1].time, .6f);
      EXPECT_EQ(edges[1].rising, true);

      EXPECT_FLOAT_EQ(edges[2].time, 1.f);
      EXPECT_EQ(edges[2].rising, false);

      EXPECT_EQ(edges[3].time, 1.6f);
      EXPECT_EQ(edges[3].rising, true);
    }

    {  // [2., 0.]
      job.from = 2.f;
      job.to = 0.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 4);

      EXPECT_FLOAT_EQ(edges[0].time, 1.6f);
      EXPECT_EQ(edges[0].rising, false);

      EXPECT_FLOAT_EQ(edges[1].time, 1.f);
      EXPECT_EQ(edges[1].rising, true);

      EXPECT_FLOAT_EQ(edges[2].time, .6f);
      EXPECT_EQ(edges[2].rising, false);

      EXPECT_EQ(edges[3].time, 0.f);
      EXPECT_EQ(edges[3].rising, true);
    }

    // Negative times

    {  // [-1, 0[
      job.from = -1.f;
      job.to = 0.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, -1.f);
      EXPECT_EQ(edges[0].rising, false);

      EXPECT_FLOAT_EQ(edges[1].time, -.4f);
      EXPECT_EQ(edges[1].rising, true);
    }

    {  // [-1.4, 0.6[
      job.from = -1.4f;
      job.to = .6f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 4);

      EXPECT_FLOAT_EQ(edges[0].time, -1.4f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, -1.f);
      EXPECT_EQ(edges[1].rising, false);

      EXPECT_FLOAT_EQ(edges[2].time, -.4f);
      EXPECT_EQ(edges[2].rising, true);

      EXPECT_EQ(edges[3].time, 0.f);
      EXPECT_EQ(edges[3].rising, false);
    }

    {  // [-1.3, 0.7[
      job.from = -1.3f;
      job.to = .7f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 4);

      EXPECT_FLOAT_EQ(edges[0].time, -1.f);
      EXPECT_EQ(edges[0].rising, false);

      EXPECT_FLOAT_EQ(edges[1].time, -.4f);
      EXPECT_EQ(edges[1].rising, true);

      EXPECT_FLOAT_EQ(edges[2].time, 0.f);
      EXPECT_EQ(edges[2].rising, false);

      EXPECT_EQ(edges[3].time, .6f);
      EXPECT_EQ(edges[3].rising, true);
    }

    // Negative backward times

    {  // [0, -1]
      job.from = 0.f;
      job.to = -1.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, -.4f);
      EXPECT_EQ(edges[0].rising, false);

      EXPECT_FLOAT_EQ(edges[1].time, -1.f);
      EXPECT_EQ(edges[1].rising, true);
    }

    {  // ]0.6, -1.5]
      job.from = .6f;
      job.to = -1.5f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 4);

      EXPECT_FLOAT_EQ(edges[0].time, 0.f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, -.4f);
      EXPECT_EQ(edges[1].rising, false);

      EXPECT_FLOAT_EQ(edges[2].time, -1.f);
      EXPECT_EQ(edges[2].rising, true);

      EXPECT_EQ(edges[3].time, -1.4f);
      EXPECT_EQ(edges[3].rising, false);
    }

    {  // ]0.7, -1.4]
      job.from = .7f;
      job.to = -1.4f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 5);

      EXPECT_FLOAT_EQ(edges[0].time, .6f);
      EXPECT_EQ(edges[0].rising, false);

      EXPECT_FLOAT_EQ(edges[1].time, 0.f);
      EXPECT_EQ(edges[1].rising, true);

      EXPECT_FLOAT_EQ(edges[2].time, -.4f);
      EXPECT_EQ(edges[2].rising, false);

      EXPECT_EQ(edges[3].time, -1.f);
      EXPECT_EQ(edges[3].rising, true);

      EXPECT_EQ(edges[4].time, -1.4f);
      EXPECT_EQ(edges[4].rising, false);
    }

    TestEdgesIntegrity(*track);

    ozz::memory::default_allocator()->Delete(track);
  }

  {  // Falling edge at t = 0.5
    ozz::animation::offline::RawFloatTrack raw_track;

    // Keyframe values oscillate in range [0,2].
    const ozz::animation::offline::RawFloatTrack::Keyframe key0 = {
        RawTrackInterpolation::kStep, 0.f, 2.f};
    raw_track.keyframes.push_back(key0);
    const ozz::animation::offline::RawFloatTrack::Keyframe key1 = {
        RawTrackInterpolation::kStep, .5f, 0.f};
    raw_track.keyframes.push_back(key1);

    // Builds track
    ozz::animation::FloatTrack* track = builder(raw_track);
    ASSERT_TRUE(track != NULL);

    FloatTrackTriggeringJob job;
    job.track = track;
    job.edges = &edges;
    int num_edges = 0;
    job.num_edges = &num_edges;
    job.threshold = 1.f;

    {  // Forward [0, .99[, 1 is excluded
      job.from = 0.f;
      job.to = .99f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, 0.f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, .5f);
      EXPECT_EQ(edges[1].rising, false);
    }

    {  // Forward [0, 1], 1 is included
      job.from = 0.f;
      job.to = 1.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, 0.f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, .5f);
      EXPECT_EQ(edges[1].rising, false);
    }

    {  // Forward [0, .5[
      job.from = 0.f;
      job.to = .5f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, 0.f);
      EXPECT_EQ(edges[0].rising, true);
    }

    {  // Forward [.1, .5[
      job.from = .1f;
      job.to = .5f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 0);
    }

    {  // Forward [.5, .9[
      job.from = .5;
      job.to = .9f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, .5f);
      EXPECT_EQ(edges[0].rising, false);
    }

    {  // Forward [.9, 1.], 1 is included
      job.from = .9f;
      job.to = 1.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 0);
    }

    {  // Forward [.5, 1.], 1 is included
      job.from = .5f;
      job.to = 1.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, .5f);
      EXPECT_EQ(edges[0].rising, false);
    }

    {  // Forward loop [0., 2.],
      job.from = 0.f;
      job.to = 2.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 4);

      EXPECT_FLOAT_EQ(edges[0].time, 0.f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, .5f);
      EXPECT_EQ(edges[1].rising, false);

      EXPECT_FLOAT_EQ(edges[2].time, 1.f);
      EXPECT_EQ(edges[2].rising, true);

      EXPECT_EQ(edges[3].time, 1.5f);
      EXPECT_EQ(edges[3].rising, false);
    }

    {  // Forward loop [0., 1.5[,
      job.from = 0.f;
      job.to = 1.5f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 3);

      EXPECT_FLOAT_EQ(edges[0].time, 0.f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, .5f);
      EXPECT_EQ(edges[1].rising, false);

      EXPECT_FLOAT_EQ(edges[2].time, 1.f);
      EXPECT_EQ(edges[2].rising, true);
    }

    {  // Forward out of bound[1.5, 1.9[
      job.from = 1.5f;
      job.to = 1.9f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, 1.5f);
      EXPECT_EQ(edges[0].rising, false);
    }

    {  // Forward out of bound[1.5, 3.]
      job.from = 1.5f;
      job.to = 3.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 3);

      EXPECT_FLOAT_EQ(edges[0].time, 1.5f);
      EXPECT_EQ(edges[0].rising, false);

      EXPECT_FLOAT_EQ(edges[1].time, 2.f);
      EXPECT_EQ(edges[1].rising, true);

      EXPECT_FLOAT_EQ(edges[2].time, 2.5f);
      EXPECT_EQ(edges[2].rising, false);
    }

    {  // Forward loop [1., 1.6[,
      job.from = 1.f;
      job.to = 1.6f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      // Creates a falling edge because it's like a loop.
      EXPECT_FLOAT_EQ(edges[0].time, 1.f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, 1.5f);
      EXPECT_EQ(edges[1].rising, false);
    }

    {  // Forward loop [.9, 1.6[,
      job.from = .9f;
      job.to = 1.6f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, 1.f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, 1.5f);
      EXPECT_EQ(edges[1].rising, false);
    }

    // Backward

    {  // Backward [1, .01], 0 is excluded
      job.from = 1.f;
      job.to = .01f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, .5f);
      EXPECT_EQ(edges[0].rising, true);
    }

    {  // Backward [1, 0], 0 is included
      job.from = 1.f;
      job.to = 0.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, .5f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, 0.f);
      EXPECT_EQ(edges[1].rising, false);
    }

    {  // Backward [2, 1],
      job.from = 2.f;
      job.to = 1.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, 1.5f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, 1.f);
      EXPECT_EQ(edges[1].rising, false);
    }

    {  // Backward ].5, 0]
      job.from = .5f;
      job.to = 0.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, 0.f);
      EXPECT_EQ(edges[0].rising, false);
    }

    {  // Backward ].9, .5]
      job.from = .9f;
      job.to = .5f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, .5f);
      EXPECT_EQ(edges[0].rising, true);
    }

    {  // Backward [1., .5]
      job.from = 1.f;
      job.to = .5f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, .5f);
      EXPECT_EQ(edges[0].rising, true);
    }

    {  // Backward ].4, .1]
      job.from = .4f;
      job.to = .1f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 0);
    }

    {  // Backward loop [2., 0.]
      job.from = 2.f;
      job.to = 0.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 4);

      EXPECT_FLOAT_EQ(edges[0].time, 1.5f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, 1.f);
      EXPECT_EQ(edges[1].rising, false);

      EXPECT_FLOAT_EQ(edges[2].time, .5f);
      EXPECT_EQ(edges[2].rising, true);

      EXPECT_EQ(edges[3].time, 0.f);
      EXPECT_EQ(edges[3].rising, false);
    }

    {  // Backward loop ]1.5, 0]
      job.from = 1.5f;
      job.to = 0.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 3);

      EXPECT_FLOAT_EQ(edges[0].time, 1.f);
      EXPECT_EQ(edges[0].rising, false);

      EXPECT_FLOAT_EQ(edges[1].time, .5f);
      EXPECT_EQ(edges[1].rising, true);

      EXPECT_FLOAT_EQ(edges[2].time, 0.f);
      EXPECT_EQ(edges[2].rising, false);
    }

    {  // Backward loop ]1.5, 1.]
      job.from = 1.5f;
      job.to = 1.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, 1.f);
      EXPECT_EQ(edges[0].rising, false);
    }

    {  // Backward loop [1.6, .9[
      job.from = 1.6f;
      job.to = .9f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, 1.5f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, 1.f);
      EXPECT_EQ(edges[1].rising, false);
    }

    {  // Backward out of bound [3., 1.5]
      job.from = 3.f;
      job.to = 1.5f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 3);

      EXPECT_FLOAT_EQ(edges[0].time, 2.5f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, 2.f);
      EXPECT_EQ(edges[1].rising, false);

      EXPECT_FLOAT_EQ(edges[2].time, 1.5f);
      EXPECT_EQ(edges[2].rising, true);
    }

    // rewind

    {  // [0., 2.]
      job.from = 0.f;
      job.to = 2.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 4);

      EXPECT_FLOAT_EQ(edges[0].time, 0.f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, .5f);
      EXPECT_EQ(edges[1].rising, false);

      EXPECT_FLOAT_EQ(edges[2].time, 1.f);
      EXPECT_EQ(edges[2].rising, true);

      EXPECT_EQ(edges[3].time, 1.5f);
      EXPECT_EQ(edges[3].rising, false);
    }

    {  // [2., 0.]
      job.from = 2.f;
      job.to = 0.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 4);

      EXPECT_FLOAT_EQ(edges[0].time, 1.5f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, 1.f);
      EXPECT_EQ(edges[1].rising, false);

      EXPECT_FLOAT_EQ(edges[2].time, .5f);
      EXPECT_EQ(edges[2].rising, true);

      EXPECT_EQ(edges[3].time, 0.f);
      EXPECT_EQ(edges[3].rising, false);
    }

    // Negative times

    {  // [-1, 0[
      job.from = -1.f;
      job.to = 0.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, -1.f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, -.5f);
      EXPECT_EQ(edges[1].rising, false);
    }

    {  // [-1.5, 0.5[
      job.from = -1.5f;
      job.to = .5f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 4);

      EXPECT_FLOAT_EQ(edges[0].time, -1.5f);
      EXPECT_EQ(edges[0].rising, false);

      EXPECT_FLOAT_EQ(edges[1].time, -1.f);
      EXPECT_EQ(edges[1].rising, true);

      EXPECT_FLOAT_EQ(edges[2].time, -.5f);
      EXPECT_EQ(edges[2].rising, false);

      EXPECT_EQ(edges[3].time, 0.f);
      EXPECT_EQ(edges[3].rising, true);
    }

    {  // [-1.4, 0.6[
      job.from = -1.4f;
      job.to = .6f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 4);

      EXPECT_FLOAT_EQ(edges[0].time, -1.f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, -.5f);
      EXPECT_EQ(edges[1].rising, false);

      EXPECT_FLOAT_EQ(edges[2].time, 0.f);
      EXPECT_EQ(edges[2].rising, true);

      EXPECT_EQ(edges[3].time, .5f);
      EXPECT_EQ(edges[3].rising, false);
    }

    // Negative backward times

    {  // [0, -1]
      job.from = 0.f;
      job.to = -1.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, -.5f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, -1.f);
      EXPECT_EQ(edges[1].rising, false);
    }

    {  // ]0.5, -1.5]
      job.from = .5f;
      job.to = -1.5f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 4);

      EXPECT_FLOAT_EQ(edges[0].time, 0.f);
      EXPECT_EQ(edges[0].rising, false);

      EXPECT_FLOAT_EQ(edges[1].time, -.5f);
      EXPECT_EQ(edges[1].rising, true);

      EXPECT_FLOAT_EQ(edges[2].time, -1.f);
      EXPECT_EQ(edges[2].rising, false);

      EXPECT_EQ(edges[3].time, -1.5f);
      EXPECT_EQ(edges[3].rising, true);
    }

    {  // ]0.6, -1.4]
      job.from = .6f;
      job.to = -1.4f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 4);

      EXPECT_FLOAT_EQ(edges[0].time, .5f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, 0.f);
      EXPECT_EQ(edges[1].rising, false);

      EXPECT_FLOAT_EQ(edges[2].time, -.5f);
      EXPECT_EQ(edges[2].rising, true);

      EXPECT_EQ(edges[3].time, -1.f);
      EXPECT_EQ(edges[3].rising, false);
    }

    TestEdgesIntegrity(*track);

    ozz::memory::default_allocator()->Delete(track);
  }
}

TEST(Linear, TrackEdgeTriggerJob) {
  TrackBuilder builder;
  FloatTrackTriggeringJob::Edge edges_buffer[8];
  ozz::Range<FloatTrackTriggeringJob::Edge> edges(edges_buffer);

  {  // Higher point at t = 0.5
    ozz::animation::offline::RawFloatTrack raw_track;

    // Keyframe values oscillate in range [0,2].
    const ozz::animation::offline::RawFloatTrack::Keyframe key0 = {
        RawTrackInterpolation::kLinear, 0.f, 0.f};
    raw_track.keyframes.push_back(key0);
    const ozz::animation::offline::RawFloatTrack::Keyframe key1 = {
        RawTrackInterpolation::kLinear, .5f, 2.f};
    raw_track.keyframes.push_back(key1);
    const ozz::animation::offline::RawFloatTrack::Keyframe key2 = {
        RawTrackInterpolation::kLinear, 1.f, 0.f};
    raw_track.keyframes.push_back(key2);

    // Builds track
    ozz::animation::FloatTrack* track = builder(raw_track);
    ASSERT_TRUE(track != NULL);

    FloatTrackTriggeringJob job;
    job.track = track;
    job.edges = &edges;
    int num_edges = 0;
    job.num_edges = &num_edges;
    job.threshold = 1.f;

    {  // Forward [0, .99[, 1 is excluded
      job.from = 0.f;
      job.to = .99f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, .25f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, .75f);
      EXPECT_EQ(edges[1].rising, false);
    }

    {  // Forward [0, 1], 1 is included
      job.from = 0.f;
      job.to = 1.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, .25f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, .75f);
      EXPECT_EQ(edges[1].rising, false);
    }

    {  // Forward [0, .5[
      job.from = 0.f;
      job.to = .5f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, .25f);
      EXPECT_EQ(edges[0].rising, true);
    }

    {  // Forward [.1, .25[
      job.from = 0.f;
      job.to = .25f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 0);
    }

    {  // Forward [.25, .5[
      job.from = 0.f;
      job.to = .5f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, .25f);
      EXPECT_EQ(edges[0].rising, true);
    }

    {  // Forward [.4, .5[
      job.from = .4f;
      job.to = .5f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 0);
    }

    {  // Forward [.5, .75[
      job.from = .5;
      job.to = .75f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 0);
    }

    {  // Forward [.75, 1.[
      job.from = .75;
      job.to = 1.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, .75f);
      EXPECT_EQ(edges[0].rising, false);
    }

    {  // Forward [.5, .9[
      job.from = .5;
      job.to = .9f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, .75f);
      EXPECT_EQ(edges[0].rising, false);
    }

    {  // Forward [.9, 1.], 1 is included
      job.from = .9f;
      job.to = 1.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 0);
    }

    {  // Forward [.5, 1.], 1 is included
      job.from = .5f;
      job.to = 1.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, .75f);
      EXPECT_EQ(edges[0].rising, false);
    }

    {  // Forward loop [0., 2.],
      job.from = 0.f;
      job.to = 2.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 4);

      EXPECT_FLOAT_EQ(edges[0].time, 0.25f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, .75f);
      EXPECT_EQ(edges[1].rising, false);

      EXPECT_FLOAT_EQ(edges[2].time, 1.25f);
      EXPECT_EQ(edges[2].rising, true);

      EXPECT_FLOAT_EQ(edges[3].time, 1.75f);
      EXPECT_EQ(edges[3].rising, false);
    }

    {  // Forward loop [0., 1.75[,
      job.from = 0.f;
      job.to = 1.75f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 3);

      EXPECT_FLOAT_EQ(edges[0].time, 0.25f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, .75f);
      EXPECT_EQ(edges[1].rising, false);

      EXPECT_FLOAT_EQ(edges[2].time, 1.25f);
      EXPECT_EQ(edges[2].rising, true);
    }

    {  // Forward out of bound[1.5, 1.9[
      job.from = 1.5f;
      job.to = 1.9f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, 1.75f);
      EXPECT_EQ(edges[0].rising, false);
    }

    {  // Forward out of bound[1.5, 3.]
      job.from = 1.5f;
      job.to = 3.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 3);

      EXPECT_FLOAT_EQ(edges[0].time, 1.75f);
      EXPECT_EQ(edges[0].rising, false);

      EXPECT_FLOAT_EQ(edges[1].time, 2.25f);
      EXPECT_EQ(edges[1].rising, true);

      EXPECT_FLOAT_EQ(edges[2].time, 2.75f);
      EXPECT_EQ(edges[2].rising, false);
    }

    {  // Forward loop [1., 1.8[,
      job.from = 1.f;
      job.to = 1.8f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      // Creates a falling edge because it's like a loop.
      EXPECT_FLOAT_EQ(edges[0].time, 1.25f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, 1.75f);
      EXPECT_EQ(edges[1].rising, false);
    }

    {  // Forward loop [1.25, 1.8[,
      job.from = 1.25f;
      job.to = 1.8f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      // Creates a falling edge because it's like a loop.
      EXPECT_FLOAT_EQ(edges[0].time, 1.25f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, 1.75f);
      EXPECT_EQ(edges[1].rising, false);
    }

    {  // Forward loop [1.25, 1.75[,
      job.from = 1.25f;
      job.to = 1.75f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, 1.25f);
      EXPECT_EQ(edges[0].rising, true);
    }

    {  // Forward loop [.9, 1.6[,
      job.from = .9f;
      job.to = 1.6f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, 1.25f);
      EXPECT_EQ(edges[0].rising, true);
    }

    // Backward

    {  // Backward [1, .01], 0 is excluded
      job.from = 1.f;
      job.to = .01f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, .75f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, .25f);
      EXPECT_EQ(edges[1].rising, false);
    }

    {  // Backward [1, 0], 0 is included
      job.from = 1.f;
      job.to = 0.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, .75f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, .25f);
      EXPECT_EQ(edges[1].rising, false);
    }

    {  // Backward [2, 1],
      job.from = 2.f;
      job.to = 1.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, 1.75f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, 1.25f);
      EXPECT_EQ(edges[1].rising, false);
    }

    {  // Backward ].5, 0]
      job.from = .5f;
      job.to = 0.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, .25f);
      EXPECT_EQ(edges[0].rising, false);
    }

    {  // Backward ].9, .5]
      job.from = .9f;
      job.to = .5f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, .75f);
      EXPECT_EQ(edges[0].rising, true);
    }

    {  // Backward [1., .75]
      job.from = 1.f;
      job.to = .75f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, .75f);
      EXPECT_EQ(edges[0].rising, true);
    }

    {  // Backward ].25, .1]
      job.from = .25f;
      job.to = .1f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 0);
    }

    {  // Backward loop [2., 0.]
      job.from = 2.f;
      job.to = 0.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 4);

      EXPECT_FLOAT_EQ(edges[0].time, 1.75f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, 1.25f);
      EXPECT_EQ(edges[1].rising, false);

      EXPECT_FLOAT_EQ(edges[2].time, .75f);
      EXPECT_EQ(edges[2].rising, true);

      EXPECT_EQ(edges[3].time, .25f);
      EXPECT_EQ(edges[3].rising, false);
    }

    {  // Backward loop ]1.5, 0]
      job.from = 1.5f;
      job.to = 0.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 3);

      EXPECT_FLOAT_EQ(edges[0].time, 1.25f);
      EXPECT_EQ(edges[0].rising, false);

      EXPECT_FLOAT_EQ(edges[1].time, .75f);
      EXPECT_EQ(edges[1].rising, true);

      EXPECT_FLOAT_EQ(edges[2].time, .25f);
      EXPECT_EQ(edges[2].rising, false);
    }

    {  // Backward loop ]1.75, 1.]
      job.from = 1.75f;
      job.to = 1.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 1);

      EXPECT_FLOAT_EQ(edges[0].time, 1.25f);
      EXPECT_EQ(edges[0].rising, false);
    }

    {  // Backward loop ]1.8, .7]
      job.from = 1.8f;
      job.to = .7f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 3);

      EXPECT_FLOAT_EQ(edges[0].time, 1.75f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, 1.25f);
      EXPECT_EQ(edges[1].rising, false);

      EXPECT_FLOAT_EQ(edges[2].time, .75f);
      EXPECT_EQ(edges[2].rising, true);
    }

    {  // Backward out of bound [3., 1.5]
      job.from = 3.f;
      job.to = 1.5f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 3);

      EXPECT_FLOAT_EQ(edges[0].time, 2.75f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, 2.25f);
      EXPECT_EQ(edges[1].rising, false);

      EXPECT_FLOAT_EQ(edges[2].time, 1.75f);
      EXPECT_EQ(edges[2].rising, true);
    }

    // rewind

    {  // [0., 1.75]
      job.from = 0.f;
      job.to = 1.75f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 3);

      EXPECT_FLOAT_EQ(edges[0].time, .25f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, .75f);
      EXPECT_EQ(edges[1].rising, false);

      EXPECT_FLOAT_EQ(edges[2].time, 1.25f);
      EXPECT_EQ(edges[2].rising, true);
    }

    {  // [1.75, 0.]
      job.from = 1.75f;
      job.to = 0.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 3);

      EXPECT_FLOAT_EQ(edges[0].time, 1.25f);
      EXPECT_EQ(edges[0].rising, false);

      EXPECT_FLOAT_EQ(edges[1].time, .75f);
      EXPECT_EQ(edges[1].rising, true);

      EXPECT_FLOAT_EQ(edges[2].time, .25f);
      EXPECT_EQ(edges[2].rising, false);
    }

    // Negative times

    {  // [-1, 0[
      job.from = -1.f;
      job.to = 0.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, -.75f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, -.25f);
      EXPECT_EQ(edges[1].rising, false);
    }

    {  // [-1.75,0.75[
      job.from = -1.75f;
      job.to = .75f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 5);

      EXPECT_FLOAT_EQ(edges[0].time, -1.75f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, -1.25f);
      EXPECT_EQ(edges[1].rising, false);

      EXPECT_FLOAT_EQ(edges[2].time, -.75f);
      EXPECT_EQ(edges[2].rising, true);

      EXPECT_FLOAT_EQ(edges[3].time, -.25f);
      EXPECT_EQ(edges[3].rising, false);

      EXPECT_FLOAT_EQ(edges[4].time, .25f);
      EXPECT_EQ(edges[4].rising, true);
    }

    {  // [-1.25, 0.8[
      job.from = -1.25f;
      job.to = .8f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 5);

      EXPECT_FLOAT_EQ(edges[0].time, -1.25f);
      EXPECT_EQ(edges[0].rising, false);

      EXPECT_FLOAT_EQ(edges[1].time, -.75f);
      EXPECT_EQ(edges[1].rising, true);

      EXPECT_FLOAT_EQ(edges[2].time, -.25f);
      EXPECT_EQ(edges[2].rising, false);

      EXPECT_EQ(edges[3].time, .25f);
      EXPECT_EQ(edges[3].rising, true);

      EXPECT_EQ(edges[4].time, .75f);
      EXPECT_EQ(edges[4].rising, false);
    }

    // Negative backward times

    {  // [0, -1]
      job.from = 0.f;
      job.to = -1.f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 2);

      EXPECT_FLOAT_EQ(edges[0].time, -.25f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, -.75f);
      EXPECT_EQ(edges[1].rising, false);
    }

    {  // ]0.5, -1.5]
      job.from = .5f;
      job.to = -1.5f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 4);

      EXPECT_FLOAT_EQ(edges[0].time, .25f);
      EXPECT_EQ(edges[0].rising, false);

      EXPECT_FLOAT_EQ(edges[1].time, -.25f);
      EXPECT_EQ(edges[1].rising, true);

      EXPECT_FLOAT_EQ(edges[2].time, -.75f);
      EXPECT_EQ(edges[2].rising, false);

      EXPECT_EQ(edges[3].time, -1.25f);
      EXPECT_EQ(edges[3].rising, true);
    }

    {  // ]0.8, -1.4]
      job.from = .8f;
      job.to = -1.4f;
      EXPECT_TRUE(job.Run());

      ASSERT_EQ(num_edges, 5);

      EXPECT_FLOAT_EQ(edges[0].time, .75f);
      EXPECT_EQ(edges[0].rising, true);

      EXPECT_FLOAT_EQ(edges[1].time, .25f);
      EXPECT_EQ(edges[1].rising, false);

      EXPECT_FLOAT_EQ(edges[2].time, -.25f);
      EXPECT_EQ(edges[2].rising, true);

      EXPECT_EQ(edges[3].time, -.75f);
      EXPECT_EQ(edges[3].rising, false);

      EXPECT_EQ(edges[4].time, -1.25f);
      EXPECT_EQ(edges[4].rising, true);
    }

    TestEdgesIntegrity(*track);

    ozz::memory::default_allocator()->Delete(track);
  }
}

TEST(StepThreshold, TrackEdgeTriggerJob) {
  TrackBuilder builder;
  FloatTrackTriggeringJob::Edge edges_buffer[8];
  ozz::Range<FloatTrackTriggeringJob::Edge> edges(edges_buffer);

  // Rising edge at t = 0.5
  ozz::animation::offline::RawFloatTrack raw_track;

  // Keyframe values oscillate in range [0,1].
  const ozz::animation::offline::RawFloatTrack::Keyframe key0 = {
      RawTrackInterpolation::kStep, 0.f, -1.f};
  raw_track.keyframes.push_back(key0);
  const ozz::animation::offline::RawFloatTrack::Keyframe key1 = {
      RawTrackInterpolation::kStep, .5f, 1.f};
  raw_track.keyframes.push_back(key1);
  const ozz::animation::offline::RawFloatTrack::Keyframe key2 = {
      RawTrackInterpolation::kStep, 1.f, -1.f};
  raw_track.keyframes.push_back(key2);

  // Builds track
  ozz::animation::FloatTrack* track = builder(raw_track);
  ASSERT_TRUE(track != NULL);

  FloatTrackTriggeringJob job;
  job.track = track;
  job.edges = &edges;
  int num_edges = 0;
  job.num_edges = &num_edges;
  job.from = 0.f;
  job.to = 1.f;

  {  // In range
    job.threshold = .5f;

    EXPECT_TRUE(job.Run());

    ASSERT_EQ(num_edges, 2);

    EXPECT_FLOAT_EQ(edges[0].time, .5f);
    EXPECT_EQ(edges[0].rising, true);
    EXPECT_FLOAT_EQ(edges[1].time, 1.f);
    EXPECT_EQ(edges[1].rising, false);
  }

  {  // Top range is included
    job.threshold = 1.f;

    EXPECT_TRUE(job.Run());

    ASSERT_EQ(num_edges, 2);

    EXPECT_FLOAT_EQ(edges[0].time, .5f);
    EXPECT_EQ(edges[0].rising, true);
    EXPECT_FLOAT_EQ(edges[1].time, 1.f);
    EXPECT_EQ(edges[1].rising, false);
  }

  {  // In range
    job.threshold = 0.f;

    EXPECT_TRUE(job.Run());

    ASSERT_EQ(num_edges, 2);

    EXPECT_FLOAT_EQ(edges[0].time, .5f);
    EXPECT_EQ(edges[0].rising, true);
    EXPECT_FLOAT_EQ(edges[1].time, 1.f);
    EXPECT_EQ(edges[1].rising, false);
  }

  {  // Bottom of range is excluded
    job.threshold = -1.f;

    EXPECT_TRUE(job.Run());

    ASSERT_EQ(num_edges, 0);
  }

  {  // Out of range
    job.from = 0.f;
    job.to = 1.f;
    job.threshold = 2.f;

    EXPECT_TRUE(job.Run());

    ASSERT_EQ(num_edges, 0);
  }

  ozz::memory::default_allocator()->Delete(track);
}

TEST(LiearThreshold, TrackEdgeTriggerJob) {
  TrackBuilder builder;
  FloatTrackTriggeringJob::Edge edges_buffer[8];
  ozz::Range<FloatTrackTriggeringJob::Edge> edges(edges_buffer);

  // Rising edge at t = 0.5
  ozz::animation::offline::RawFloatTrack raw_track;

  // Keyframe values oscillate in range [0,1].
  const ozz::animation::offline::RawFloatTrack::Keyframe key0 = {
      RawTrackInterpolation::kLinear, 0.f, -1.f};
  raw_track.keyframes.push_back(key0);
  const ozz::animation::offline::RawFloatTrack::Keyframe key1 = {
      RawTrackInterpolation::kLinear, .5f, 1.f};
  raw_track.keyframes.push_back(key1);
  const ozz::animation::offline::RawFloatTrack::Keyframe key2 = {
      RawTrackInterpolation::kLinear, 1.f, -1.f};
  raw_track.keyframes.push_back(key2);

  // Builds track
  ozz::animation::FloatTrack* track = builder(raw_track);
  ASSERT_TRUE(track != NULL);

  FloatTrackTriggeringJob job;
  job.track = track;
  job.edges = &edges;
  int num_edges = 0;
  job.num_edges = &num_edges;
  job.from = 0.f;
  job.to = 1.f;

  {  // In range
    job.threshold = .5f;

    EXPECT_TRUE(job.Run());

    ASSERT_EQ(num_edges, 2);

    EXPECT_FLOAT_EQ(edges[0].time, .375f);
    EXPECT_EQ(edges[0].rising, true);
    EXPECT_FLOAT_EQ(edges[1].time, .625f);
    EXPECT_EQ(edges[1].rising, false);
  }

  {  // Top range is included
    job.threshold = 1.f;

    EXPECT_TRUE(job.Run());

    ASSERT_EQ(num_edges, 2);

    EXPECT_FLOAT_EQ(edges[0].time, .5f);
    EXPECT_EQ(edges[0].rising, true);
    EXPECT_FLOAT_EQ(edges[1].time, .5f);
    EXPECT_EQ(edges[1].rising, false);
  }

  {  // In range
    job.threshold = 0.f;

    EXPECT_TRUE(job.Run());

    ASSERT_EQ(num_edges, 2);

    EXPECT_FLOAT_EQ(edges[0].time, .25f);
    EXPECT_EQ(edges[0].rising, true);
    EXPECT_FLOAT_EQ(edges[1].time, .75f);
    EXPECT_EQ(edges[1].rising, false);
  }

  {  // Bottom of range is excluded
    job.threshold = -1.f;

    EXPECT_TRUE(job.Run());

    ASSERT_EQ(num_edges, 0);
  }

  {  // Out of range
    job.from = 0.f;
    job.to = 1.f;
    job.threshold = 2.f;

    EXPECT_TRUE(job.Run());

    ASSERT_EQ(num_edges, 0);
  }

  ozz::memory::default_allocator()->Delete(track);
}