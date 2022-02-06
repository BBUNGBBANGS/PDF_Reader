#if 0
template <int LEN, int SUM, bool IS_SPARCE>
PatternView FindLeftGuard(const PatternView& view, int minSize, const FixedPattern<LEN, SUM, IS_SPARCE>& pattern,
						  float minQuiteZone)
{
	return FindLeftGuard<LEN>(view, std::max(minSize, LEN),
							  [&pattern, minQuiteZone](const PatternView& window, int spaceInPixel) {
								  return IsPattern(window, pattern, spaceInPixel, minQuiteZone);
							  });
}
#endif
