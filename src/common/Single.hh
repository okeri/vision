// Copyright 2017 Keri Oleg

#pragma once

class Single {
    Single(const Single&) = delete;
    Single(Single&&) = delete;
    Single& operator=(const Single &) = delete;

  protected:
    Single() = default;
};
