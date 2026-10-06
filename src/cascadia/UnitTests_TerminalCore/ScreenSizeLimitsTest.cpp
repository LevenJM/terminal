// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"
#include <WexTestClass.h>

#include "../cascadia/TerminalCore/Terminal.hpp"
#include "MockTermSettings.h"
#include "../renderer/inc/DummyRenderer.hpp"
#include "consoletaeftemplates.hpp"

using namespace winrt::Microsoft::Terminal::Core;
using namespace Microsoft::Terminal::Core;
using namespace WEX::Logging;
using namespace WEX::TestExecution;
using namespace WEX::Common;

namespace TerminalCoreUnitTests
{
#define WCS(x) WCSHELPER(x)
#define WCSHELPER(x) L## #x

    class ScreenSizeLimitsTest
    {
        TEST_CLASS(ScreenSizeLimitsTest);

        TEST_METHOD(ScreenWidthAndHeightAreClampedToBounds);
        TEST_METHOD(ScrollbackHistorySizeIsClampedToBounds);

        TEST_METHOD(ResizeIsClampedToBounds);
    };
}

using namespace TerminalCoreUnitTests;

void ScreenSizeLimitsTest::ScreenWidthAndHeightAreClampedToBounds()
{
    // Negative values for initial visible row count or column count
    // are clamped to 1. Too-large positive values are clamped to SHRT_MAX.
    auto negativeColumnsSettings = winrt::make<MockTermSettings>(10000, 9999999, -1234);
    Terminal negativeColumnsTerminal{ Terminal::TestDummyMarker{} };
    DummyRenderer renderer{ &negativeColumnsTerminal };
    negativeColumnsTerminal.CreateFromSettings(negativeColumnsSettings, renderer);
    auto actualDimensions = negativeColumnsTerminal.GetViewport().Dimensions();
    VERIFY_ARE_EQUAL(actualDimensions.height, SHRT_MAX, L"Row count clamped to SHRT_MAX == " WCS(SHRT_MAX));
    VERIFY_ARE_EQUAL(actualDimensions.width, 1, L"Column count clamped to 1");

    // Zero values are clamped to 1 as well.
    auto zeroRowsSettings = winrt::make<MockTermSettings>(10000, 0, 9999999);
    Terminal zeroRowsTerminal{ Terminal::TestDummyMarker{} };
    zeroRowsTerminal.CreateFromSettings(zeroRowsSettings, renderer);
    actualDimensions = zeroRowsTerminal.GetViewport().Dimensions();
    VERIFY_ARE_EQUAL(actualDimensions.height, 1, L"Row count clamped to 1");
    VERIFY_ARE_EQUAL(actualDimensions.width, SHRT_MAX, L"Column count clamped to SHRT_MAX == " WCS(SHRT_MAX));
}

void ScreenSizeLimitsTest::ScrollbackHistorySizeIsClampedToBounds()
{
    // What is actually clamped is the number of rows in the internal history buffer,
    // which is the *sum* of the history size plus the number of rows
    // actually visible on screen at the moment.

    static constexpr til::CoordType visibleRowCount = 100;

    // Zero history size is acceptable.
    auto noHistorySettings = winrt::make<MockTermSettings>(0, visibleRowCount, 100);
    Terminal noHistoryTerminal{ Terminal::TestDummyMarker{} };
    DummyRenderer renderer{ &noHistoryTerminal };
    noHistoryTerminal.CreateFromSettings(noHistorySettings, renderer);
    VERIFY_ARE_EQUAL(noHistoryTerminal.GetTextBuffer().TotalRowCount(), visibleRowCount, L"History size of 0 is accepted");

    // Negative history sizes are clamped to zero.
    auto negativeHistorySizeSettings = winrt::make<MockTermSettings>(-100, visibleRowCount, 100);
    Terminal negativeHistorySizeTerminal{ Terminal::TestDummyMarker{} };
    negativeHistorySizeTerminal.CreateFromSettings(negativeHistorySizeSettings, renderer);
    VERIFY_ARE_EQUAL(negativeHistorySizeTerminal.GetTextBuffer().TotalRowCount(), visibleRowCount, L"Negative history size is clamped to 0");

    // History size + initial visible rows == MAXIMUM_BUFFER_HEIGHT is acceptable.
    auto maxHistorySizeSettings = winrt::make<MockTermSettings>(MAXIMUM_BUFFER_HEIGHT - visibleRowCount, visibleRowCount, 100);
    Terminal maxHistorySizeTerminal{ Terminal::TestDummyMarker{} };
    maxHistorySizeTerminal.CreateFromSettings(maxHistorySizeSettings, renderer);
    VERIFY_ARE_EQUAL(maxHistorySizeTerminal.GetTextBuffer().TotalRowCount(), MAXIMUM_BUFFER_HEIGHT, L"History size == MAXIMUM_BUFFER_HEIGHT - initial row count is accepted");

    // History size + initial visible rows == MAXIMUM_BUFFER_HEIGHT + 1 will be clamped slightly.
    auto justTooBigHistorySizeSettings = winrt::make<MockTermSettings>(MAXIMUM_BUFFER_HEIGHT - visibleRowCount + 1, visibleRowCount, 100);
    Terminal justTooBigHistorySizeTerminal{ Terminal::TestDummyMarker{} };
    justTooBigHistorySizeTerminal.CreateFromSettings(justTooBigHistorySizeSettings, renderer);
    VERIFY_ARE_EQUAL(justTooBigHistorySizeTerminal.GetTextBuffer().TotalRowCount(), MAXIMUM_BUFFER_HEIGHT, L"History size == 1 + MAXIMUM_BUFFER_HEIGHT - initial row count is clamped to MAXIMUM_BUFFER_HEIGHT - initial row count");

    // Ridiculously large history sizes are also clamped.
    auto farTooBigHistorySizeSettings = winrt::make<MockTermSettings>(INT_MAX, visibleRowCount, 100);
    Terminal farTooBigHistorySizeTerminal{ Terminal::TestDummyMarker{} };
    farTooBigHistorySizeTerminal.CreateFromSettings(farTooBigHistorySizeSettings, renderer);
    VERIFY_ARE_EQUAL(farTooBigHistorySizeTerminal.GetTextBuffer().TotalRowCount(), MAXIMUM_BUFFER_HEIGHT, L"History size that is far too large is clamped to MAXIMUM_BUFFER_HEIGHT - initial row count");
}

void ScreenSizeLimitsTest::ResizeIsClampedToBounds()
{
    // What is actually clamped is the number of rows in the internal history buffer,
    // which is the *sum* of the history size plus the number of rows
    // actually visible on screen at the moment.
    //
    // This is a test for GH#2630, GH#2815.

    static constexpr til::CoordType initialVisibleColCount = 50;
    static constexpr til::CoordType initialVisibleRowCount = 50;
    const auto historySize = MAXIMUM_BUFFER_HEIGHT - (initialVisibleRowCount * 2);

    Log::Comment(L"Watch out - this test takes a while on debug, because "
                 L"ResizeWithReflow takes a while on debug. This is expected.");

    auto settings = winrt::make<MockTermSettings>(historySize, initialVisibleRowCount, initialVisibleColCount);
    Log::Comment(L"First create a terminal with fewer than MAXIMUM_BUFFER_HEIGHT lines");
    Terminal terminal{ Terminal::TestDummyMarker{} };
    DummyRenderer renderer{ &terminal };
    terminal.CreateFromSettings(settings, renderer);
    VERIFY_ARE_EQUAL(terminal.GetTextBuffer().TotalRowCount(), historySize + initialVisibleRowCount);

    Log::Comment(L"Resize the terminal to have exactly MAXIMUM_BUFFER_HEIGHT lines");
    VERIFY_SUCCEEDED(terminal.UserResize({ initialVisibleColCount, initialVisibleRowCount * 2 }));

    VERIFY_ARE_EQUAL(terminal.GetTextBuffer().TotalRowCount(), MAXIMUM_BUFFER_HEIGHT);

    Log::Comment(L"Resize the terminal to have MORE than MAXIMUM_BUFFER_HEIGHT lines - we should clamp to MAXIMUM_BUFFER_HEIGHT");
    VERIFY_SUCCEEDED(terminal.UserResize({ initialVisibleColCount, initialVisibleRowCount * 3 }));
    VERIFY_ARE_EQUAL(terminal.GetTextBuffer().TotalRowCount(), MAXIMUM_BUFFER_HEIGHT);

    Log::Comment(L"Resize back down to the original size");
    VERIFY_SUCCEEDED(terminal.UserResize({ initialVisibleColCount, initialVisibleRowCount }));
    VERIFY_ARE_EQUAL(terminal.GetTextBuffer().TotalRowCount(), historySize + initialVisibleRowCount);
}
