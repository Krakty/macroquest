/*
 * MQ2CF: MacroQuest Class Framework
 * Humanization bindings -- timing jitter and randomization to avoid bot detection
 *
 * Provides bell-curve delays, jittered timings, and random skip logic so that
 * automated actions don't fire with machine precision.  All RNG uses a
 * thread_local mt19937 seeded from std::random_device.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.
 */

#include <mq/Plugin.h>
#include "Humanize.h"

#include <random>
#include <algorithm>
#include <cmath>

namespace CF {

// ---------------------------------------------------------------------------
// RNG engine -- one per thread, seeded once from hardware entropy
// ---------------------------------------------------------------------------

static std::mt19937& GetRNG()
{
	static thread_local std::mt19937 rng{ std::random_device{}() };
	return rng;
}

// ---------------------------------------------------------------------------
// Core.Humanize bindings
// ---------------------------------------------------------------------------

void RegisterHumanizeBindings(sol::table& core)
{
	sol::table humanize = core["Humanize"].get_or_create<sol::table>();

	// Jitter(baseMs, variancePct) -> int
	// Returns baseMs +/- variancePct% random variation.
	// E.g. Jitter(1000, 20) returns a value in [800, 1200].
	humanize.set_function("Jitter", [](int baseMs, double variancePct) -> int {
		if (variancePct <= 0.0)
			return baseMs;

		double fraction = variancePct / 100.0;
		double lo = baseMs * (1.0 - fraction);
		double hi = baseMs * (1.0 + fraction);

		std::uniform_real_distribution<double> dist(lo, hi);
		return static_cast<int>(std::round(dist(GetRNG())));
	});

	// HumanDelay(minMs, maxMs) -> int
	// Bell-curve weighted random delay between min and max.
	// Uses the average of two uniform samples to approximate a triangle
	// distribution that clusters toward the midpoint.
	humanize.set_function("HumanDelay", [](int minMs, int maxMs) -> int {
		if (minMs >= maxMs)
			return minMs;

		std::uniform_real_distribution<double> dist(
			static_cast<double>(minMs),
			static_cast<double>(maxMs));

		// Average of two uniform draws gives a triangle (tent) distribution
		// centered on the midpoint -- simple, cheap, no tuning knobs.
		double a = dist(GetRNG());
		double b = dist(GetRNG());
		return static_cast<int>(std::round((a + b) / 2.0));
	});

	// ShouldSkipTick(skipChancePct) -> bool
	// Returns true skipChancePct% of the time.
	humanize.set_function("ShouldSkipTick", [](double skipChancePct) -> bool {
		if (skipChancePct <= 0.0)
			return false;
		if (skipChancePct >= 100.0)
			return true;

		std::uniform_real_distribution<double> dist(0.0, 100.0);
		return dist(GetRNG()) < skipChancePct;
	});

	// RandomFloat(min, max) -> float
	// Simple uniform random float in [min, max].
	humanize.set_function("RandomFloat", [](float minVal, float maxVal) -> float {
		if (minVal >= maxVal)
			return minVal;

		std::uniform_real_distribution<float> dist(minVal, maxVal);
		return dist(GetRNG());
	});

	// WeightedRandom(min, max, weight) -> int
	// weight 0.0 = uniform distribution
	// weight 1.0 = heavily center-weighted
	// Intermediate values blend between uniform and center-weighted.
	// Uses averaging of multiple uniform samples -- more averages = tighter center.
	humanize.set_function("WeightedRandom", [](int minVal, int maxVal, double weight) -> int {
		if (minVal >= maxVal)
			return minVal;

		std::uniform_real_distribution<double> dist(
			static_cast<double>(minVal),
			static_cast<double>(maxVal));

		// Clamp weight to [0, 1]
		weight = std::clamp(weight, 0.0, 1.0);

		if (weight <= 0.001)
		{
			// Pure uniform
			return static_cast<int>(std::round(dist(GetRNG())));
		}

		// Blend: take a uniform sample and a center-weighted sample (average
		// of 4 draws), then lerp between them based on weight.
		double uniform = dist(GetRNG());

		double sum = 0.0;
		for (int i = 0; i < 4; ++i)
			sum += dist(GetRNG());
		double centered = sum / 4.0;

		double result = uniform * (1.0 - weight) + centered * weight;
		return static_cast<int>(std::round(result));
	});
}

} // namespace CF
