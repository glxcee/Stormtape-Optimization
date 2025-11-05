#include "extended_file_status.hpp"

namespace storm {

bool ExtendedFileStatus::is_in_progress()
{
  bool ret{false};
  if (m_in_progress.has_value()) {
    ret = *m_in_progress;
  } else {
    if (auto result = m_storage.is_in_progress(m_path)) {
      m_in_progress = *result;
      ret           = *m_in_progress;
    } else {
      // log something
      m_ec = result.error();
    }
  }
  return ret;
}

bool ExtendedFileStatus::is_stub()
{
  bool ret{false};
  if (m_file_size_info.has_value()) {
    ret = m_file_size_info->is_stub;
  } else {
    if (auto result = m_storage.file_size_info(m_path)) {
      m_file_size_info = *result;
      ret              = m_file_size_info->is_stub;
    } else {
      // log something
      m_ec = result.error();
    }
  }
  return ret;
}

bool ExtendedFileStatus::is_on_tape()
{
  bool ret{false};
  if (m_on_tape.has_value()) {
    ret = *m_on_tape;
  } else {
    if (auto result = m_storage.is_on_tape(m_path)) {
      m_on_tape = *result;
      ret       = *m_on_tape;
    } else {
      // log something
      m_ec = result.error();
    }
  }
  return ret;
}

std::size_t ExtendedFileStatus::file_size()
{
  std::size_t ret{0};
  if (m_file_size_info.has_value()) {
    ret = m_file_size_info->size;
  } else {
    if (auto result = m_storage.file_size_info(m_path)) {
      m_file_size_info = *result;
      ret              = m_file_size_info->size;
    } else {
      // log something
      m_ec = result.error();
    }
  }
  return ret;
}

Locality ExtendedFileStatus::locality()
{
  bool const is_on_disk_{!(is_stub() || is_in_progress())};
  bool const is_on_tape_{is_on_tape()};

  if (error() != std::error_code{}) {
    return Locality::unavailable;
  }

  if (is_on_disk_) {
    return is_on_tape_ ? Locality::disk_and_tape : Locality::disk;
  } else {
    return is_on_tape_ ? Locality::tape : Locality::lost;
  }
}

} // namespace storm
