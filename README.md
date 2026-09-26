# pm_settings

ROS 2 пакет для управления настройками манипулятора Promobot M13 в режиме реального времени, разработанный в рамках выпускной квалификационной работы студента ПНИПУ, ЭТФ, ИТАС, гр. РИС-22-1б, Деревнин Илья.  
Предоставляет сервисы для управления смещением TCP, трансформацией базового звена, ограничениями суставов, объектами сцены и рабочими системами координат — с валидацией, персистентностью и автоматическим откатом при сбое.

## Обзор

Модуль построен вокруг четырёхэтапного pipeline применения параметра:

```
Валидация → Применение → Сохранение → Подтверждение
```

Если любой этап после *Применения* завершается с ошибкой, обработчик автоматически откатывает изменения к предыдущему состоянию.

**Ключевые компоненты:**

| Слой | Класс | Ответственность |
|---|---|---|
| ROS 2 интерфейс | `SettingsServiceNode` | Приём сервисных вызовов, сериализация/десериализация сообщений |
| Бизнес-логика | `SettingsManager` | Оркестрация четырёхэтапного pipeline |
| Обработчики | `TCPHandler`, `MountingHandler`, `JointLimitsHandler`, `SceneHandler` | Применение параметров к runtime-компонентам MControl |
| Валидация | `ValidatorRegistry` + `I*Validator` | Безсостоятельная валидация по типу параметра |
| Персистентность | `ConfigFileManager` + JSON/YAML сериализаторы | Чтение и запись конфигурационных файлов |

## Конфигурируемые параметры

| `ParamType` | Описание |
|---|---|
| `TCP` | Смещение центральной точки инструмента (до 5 именованных конфигураций) |
| `MOUNTING` | Трансформация базового звена (поза монтажа манипулятора) |
| `JOINT_LIMITS` | Ограничения положения, скорости и ускорения каждого сустава |
| `HOME_POSITION` | Начальные углы суставов (домашняя позиция) |
| `SCENE_OBJECT` | Препятствия в пространстве планирования (куб, цилиндр, сфера, стена, …) |
| `WORK_FRAME` | Активная рабочая система координат |

## ROS 2 Сервисы

### TCP
| Сервис | Тип |
|---|---|
| `~/tcp/add` | `core_msgs/srv/AddTCP` |
| `~/tcp/apply` | `core_msgs/srv/ApplyTCP` |
| `~/tcp/delete` | `core_msgs/srv/DeleteTCP` |
| `~/tcp/get_list` | `std_srvs/srv/Trigger` |
| `~/tcp/get_current` | `std_srvs/srv/Trigger` |
| `~/tcp/reset` | `std_srvs/srv/Trigger` |

### Базовое звено (монтаж)
| Сервис | Тип |
|---|---|
| `~/base_link/add` | `core_msgs/srv/AddTransform` |
| `~/base_link/apply` | `core_msgs/srv/ApplyTransform` |
| `~/base_link/delete` | `core_msgs/srv/DeleteTransform` |
| `~/base_link/get_list` | `std_srvs/srv/Trigger` |
| `~/base_link/get_current` | `std_srvs/srv/Trigger` |
| `~/base_link/reset` | `std_srvs/srv/Trigger` |

### Объекты сцены
| Сервис | Тип |
|---|---|
| `~/objects/add` | `core_msgs/srv/AddObject` |
| `~/objects/get` | `core_msgs/srv/GetObject` |
| `~/objects/delete` | `core_msgs/srv/DeleteObject` |
| `~/objects/list` | `core_msgs/srv/GetObjectNames` |
| `~/objects/update_scene` | `std_srvs/srv/Trigger` |

### Рабочая система координат
| Сервис | Тип |
|---|---|
| `~/work_frame/add` | `core_msgs/srv/AddWorkFrame` |
| `~/work_frame/apply` | `core_msgs/srv/ApplyWorkFrame` |
| `~/work_frame/delete` | `core_msgs/srv/DeleteWorkFrame` |
| `~/work_frame/list` | `core_msgs/srv/ListWorkFrame` |
| `~/work_frame/get_current` | `core_msgs/srv/GetWorkFrame` |
| `~/work_frame/reset` | `std_srvs/srv/Trigger` |

## Зависимости

- ROS 2 (Humble или новее)
- `rclcpp`, `std_srvs`, `geometry_msgs`, `shape_msgs`
- `moveit_msgs`, `moveit_ros_planning_interface`
- `tf2`, `tf2_ros`, `tf2_geometry_msgs`, `visualization_msgs`
- `core_msgs`, `core_utils` (внутренние пакеты Promobot)
- `yaml-cpp`, `fmt`, `Boost::json`

## Сборка

```bash
# из корня ROS 2 workspace
colcon build --packages-select pm_settings
source install/setup.bash
```

## Запуск

```bash
ros2 run pm_settings pm_settings_node
```

## Конфигурационные файлы

Файлы по умолчанию устанавливаются в `share/pm_settings/config/`:

| Файл | Содержимое |
|---|---|
| `tcp_list.json` | Сохранённые конфигурации TCP |
| `base_link_transforms.json` | Сохранённые трансформации монтажа |
| `objects.json` | Объекты сцены |
| `work_frame_list.json` | Конфигурации рабочих систем координат |

## Архитектура

```
SettingsServiceNode  (ROS 2 слой — без бизнес-логики)
        │
        ▼
SettingsManager  (оркестратор)
   ├── ValidatorRegistry  ──►  TCPValidator
   │                           MountingValidator
   │                           JointLimitsValidator
   │                           SceneObjectValidator
   │
   ├── ISettingsHandler   ──►  TCPHandler
   │                           MountingHandler
   │                           JointLimitsHandler
   │                           SceneHandler
   │
   └── ConfigFileManager  ──►  JsonSerializer
                                YamlSerializer
```

Каждый обработчик следует единому контракту:
- `Apply()` — сохранить текущее состояние, затем применить новое значение
- `Rollback()` — восстановить сохранённое состояние при сбое последующего этапа
- `GetCurrent()` — вернуть активное значение

## Лицензия

Проприетарная — Copyright © 2015–2026 Promobot LLC. Все права защищены.
