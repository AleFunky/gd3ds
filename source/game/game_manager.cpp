#include "game_manager.hpp"
#include <memory>

namespace gd3ds
{
    std::unique_ptr<GameManager> GameManager::s_instance = nullptr;

}