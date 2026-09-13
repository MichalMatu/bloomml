#include "climate/display/DisplayAsyncTransaction.h"

#include <cassert>
#include <cstring>

namespace display = growbox::app::climate_io::display;

int main() {
  display::DisplayAsyncTransaction transaction;
  display::DisplayRuntimeFrame source{};
  source.refresh_kind = display::DisplayRefreshKind::Full;
  source.refresh_reason = display::DisplayRefreshReason::Initial;
  source.render_list.command_count = 1U;
  source.render_list.commands[0].text[0] = 'A';
  source.render_list.commands[0].text[1] = '\0';

  display::DisplayRenderWorkItem first{};
  assert(transaction.start(source, first));
  assert(transaction.inFlight());
  assert(first.generation != 0U);
  assert(first.frame.refresh_kind == display::DisplayRefreshKind::Full);
  assert(std::strcmp(first.frame.render_list.commands[0].text.data(), "A") == 0);

  source.refresh_kind = display::DisplayRefreshKind::Partial;
  source.render_list.commands[0].text[0] = 'B';
  assert(std::strcmp(transaction.inFlightFrame().render_list.commands[0].text.data(), "A") == 0);

  display::DisplayRenderWorkItem rejected{};
  assert(!transaction.start(source, rejected));

  const display::DisplayRenderCompletion stale{first.generation + 1U, 100U, true};
  assert(!transaction.matches(stale));
  assert(!transaction.finish(stale));
  assert(transaction.inFlight());

  const display::DisplayRenderCompletion first_done{first.generation, 123U, true};
  assert(transaction.matches(first_done));
  assert(transaction.finish(first_done));
  assert(!transaction.inFlight());

  display::DisplayRenderWorkItem second{};
  assert(transaction.start(source, second));
  assert(second.generation != 0U);
  assert(second.generation != first.generation);
  assert(second.frame.refresh_kind == display::DisplayRefreshKind::Partial);
  transaction.abort();
  assert(!transaction.inFlight());

  display::DisplayRuntimeFrame no_refresh{};
  no_refresh.refresh_kind = display::DisplayRefreshKind::None;
  display::DisplayRenderWorkItem none{};
  assert(!transaction.start(no_refresh, none));

  return 0;
}
