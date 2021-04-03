#pragma once

namespace stdml::collective
{
// TODO: export top level functions
void init();
void finalize();
void resize();

void all_gather();

void all_reduce();

void broadcast();

void gather();

void reduce();
}  // namespace stdml::collective
