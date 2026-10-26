# RatRace — контекст проекта

Документ для разработчика, который впервые открывает проект. Описывает фактическое состояние кода на момент последнего обновления (добавление хвостов, октябрь 2026). Если раздел помечен **«не реализовано»**, значит в коде этого нет, есть только замысел.

---

## 1. Назначение игры

- **Концепция.** RatRace — локальная party game. Крысы сидят в коробке и соревнуются друг с другом в серии мини-игр.
- **Игроки.** От 1 до 4 человек за одним экраном (без сплитскрина), у каждого свой геймпад и своя крыса.
- **Цель матча.** По итогам мини-игр определяется лучший игрок. Он получает награду (сыр) и становится победителем.
- **Мини-игры** задуманы как модули, которые можно добавлять и развивать независимо друг от друга.
- **Хвосты.** У каждой крысы есть хвост. Другой игрок может схватить его и тянуть крысу за собой. Это базовое взаимодействие между игроками, на котором могут строиться мини-игры.

**Что из этого есть в коде сейчас:** локальный мультиплеер на 4 крысы, общий экран, раздельные геймпады, движение и прыжки, тестовая арена, хвосты с захватом и перетягиванием. Коробки как уровня, мини-игр, подсчёта очков, победителя и сыра в коде **нет** (см. §5 и §7).

---

## 2. Техническая архитектура

| Аспект | Факт |
|---|---|
| Движок | Unreal Engine **5.7.4** (`EngineAssociation: "5.7"` в [RatRace.uproject](RatRace.uproject)) |
| Язык | C++ (модуль `RatRace`). Активный игровой пайплайн целиком на C++; Blueprint-ассеты в `Content/coop` остались от раннего прототипа и текущим GameMode не используются (см. §3.3) |
| Ввод | Enhanced Input. Input Actions и Mapping Context **создаются в рантайме в C++** (`NewObject`), отдельно для каждого PlayerController. `.uasset` для ввода не нужны |
| Мультиплеер | Только локальный: несколько `ULocalPlayer` в одном процессе. Сетевого кода нет |
| Камера | Одна фиксированная `ACameraActor` над ареной, все контроллеры смотрят через неё. Сплитскрин принудительно выключен |
| Уровень | Карта по умолчанию — `/Game/coop/maps/L_RatRace` (пустая). Пол, платформы, камеру и свет строит `ARatRaceArenaBuilder` в рантайме |
| GameMode | `GlobalDefaultGameMode=/Script/RatRace.RatRaceGameMode` в [Config/DefaultEngine.ini](Config/DefaultEngine.ini) |
| GameInstance | Не задан в конфиге, используется стандартный `UGameInstance` |
| Зависимости модуля | `Core, CoreUObject, Engine, InputCore, EnhancedInput` (public); `ApplicationCore, Slate, SlateCore` (private), см. [RatRace.Build.cs](Source/RatRace/RatRace.Build.cs) |

### Основные классы

| Класс | Файл | Ответственность |
|---|---|---|
| `ARatRaceGameMode` | [RatRaceGameMode.h](Source/RatRace/RatRaceGameMode.h) / [.cpp](Source/RatRace/RatRaceGameMode.cpp) | Создаёт локальных игроков, привязывает геймпады к игрокам, спавнит крыс в фиксированных точках, красит их, возвращает упавших на точку спавна, отдаёт общую камеру |
| `ARatRacePlayerController` | [RatRacePlayerController.h](Source/RatRace/RatRacePlayerController.h) / [.cpp](Source/RatRace/RatRacePlayerController.cpp) | Один на локального игрока. Создаёт свои Input Actions и IMC, переводит ввод в движение, прыжок и захват хвоста, держит view target на камере арены |
| `ARatRaceCharacter` | [RatRaceCharacter.h](Source/RatRace/RatRaceCharacter.h) / [.cpp](Source/RatRace/RatRaceCharacter.cpp) | Крыса: капсула и `CharacterMovementComponent`, визуал из базовых фигур движка (тело-цилиндр, нос-куб, хвост-цилиндр), цвет игрока, компонент хвоста |
| `URatRaceTailComponent` | [RatRaceTailComponent.h](Source/RatRace/RatRaceTailComponent.h) / [.cpp](Source/RatRace/RatRaceTailComponent.cpp) | Кончик хвоста. Отвечает за захват чужого хвоста, тягу и освобождение (§6) |
| `ARatRaceArenaBuilder` | [RatRaceArenaBuilder.h](Source/RatRace/RatRaceArenaBuilder.h) / [.cpp](Source/RatRace/RatRaceArenaBuilder.cpp) | Строит тестовую арену в рантайме: пол, платформы, камеру с автоподбором дистанции, свет и небо, если их нет на уровне |
| `FRatRaceGamepadRouter` | [RatRaceGamepadRouter.h](Source/RatRace/RatRaceGamepadRouter.h) / [.cpp](Source/RatRace/RatRaceGamepadRouter.cpp) | Slate input preprocessor. Передаёт события всех геймпадов прямо в `UGameViewportClient`, минуя фокус Slate, иначе пады игроков 1–3 не доходят до игры |
| `UCoopGameInstance` | [CoopGameInstance.h](Source/RatRace/CoopGameInstance.h) / [.cpp](Source/RatRace/CoopGameInstance.cpp) | Наследие прототипа: `TryJoinNextPlayer` и счётчики игроков. Активным пайплайном **не используется** (в конфиге не назначен) |

### Как связаны системы

```
UGameInstance
 └─ ULocalPlayer[0..3]  (PlatformUserId ← геймпад N, назначает ARatRaceGameMode)
     └─ ARatRacePlayerController (свой IA_Move / IA_Jump / IA_Grab + IMC_RatRace)
         └─ possess → ARatRaceCharacter
                       ├─ CharacterMovementComponent (ходьба, прыжок, поворот по движению)
                       ├─ BodyMesh / NoseMesh / TailMesh (визуал, без коллизии)
                       └─ URatRaceTailComponent "Tail" (кончик хвоста + логика захвата)

Slate → FRatRaceGamepadRouter → UGameViewportClient::InputKey/InputAxis
      → ULocalPlayer по PlatformUserId устройства → его PlayerController → Enhanced Input
```

---

## 3. Структура файлов и папок

### 3.1. Корень

| Путь | Назначение |
|---|---|
| [RatRace.uproject](RatRace.uproject) | Проект UE 5.7, модуль `RatRace`, плагины `EnhancedInput`, `ModelingToolsEditorMode` (только редактор) |
| [Config/DefaultEngine.ini](Config/DefaultEngine.ini) | Карта по умолчанию `L_RatRace`, GameMode `RatRaceGameMode`, `bUseSplitscreen=False`, `bOffsetPlayerGamepadIds=False`, настройки рендера |
| [Config/DefaultInput.ini](Config/DefaultInput.ini) | `DefaultPlayerInputClass=EnhancedPlayerInput`, `DefaultInputComponentClass=EnhancedInputComponent`, `bFilterInputByPlatformUser=False`. Legacy action/axis mappings не используются |
| `Config/DefaultGame.ini`, `Config/DefaultEditor.ini` | Стандартные, ничего специфичного для игры |
| `PROJECT_CONTEXT.md` | Этот документ |
| `UpgradeLog.htm` | Лог конвертации решения Visual Studio, к игре отношения не имеет |
| `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/` | Генерируются при сборке и запуске, в git не хранятся |

### 3.2. Исходный код — `Source/RatRace/`

Все игровые классы лежат в одном модуле, плоско:

| Файл | Связи |
|---|---|
| `RatRace.Build.cs`, `RatRace.h/.cpp` | Описание и регистрация модуля |
| `RatRaceGameMode.*` | Использует `ARatRaceArenaBuilder` (арена и камера), `ARatRaceCharacter` (DefaultPawnClass), `ARatRacePlayerController` (PlayerControllerClass), `FRatRaceGamepadRouter` |
| `RatRacePlayerController.*` | Берёт камеру у `ARatRaceGameMode::GetArenaCamera()`, ищет на пауне `URatRaceTailComponent` для захвата |
| `RatRaceCharacter.*` | Создаёт `URatRaceTailComponent` и `TailMesh` |
| `RatRaceTailComponent.*` | Работает с любым `APawn`/`ACharacter`, у которого есть этот компонент; от других классов проекта не зависит |
| `RatRaceArenaBuilder.*` | Самостоятельный класс; GameMode находит его на уровне или спавнит |
| `RatRaceGamepadRouter.*` | Не UObject, создаётся и регистрируется в `ARatRaceGameMode::BeginPlay` |
| `CoopGameInstance.*` | Используется только Blueprint-ассетом `BP_CoopGameInstance` (прототип) |
| `Source/RatRace.Target.cs`, `Source/RatRaceEditor.Target.cs` | Таргеты сборки Game и Editor |

### 3.3. Контент — `Content/coop/`

| Путь | Что это | Используется активным пайплайном? |
|---|---|---|
| `maps/L_RatRace.umap` | Пустая карта по умолчанию, всё строится в C++ | **Да** |
| `maps/L_CoopTest.umap` | Ранняя тестовая карта (пол, PlayerStart ×4, камера, свет) | Нет (ссылок на BP-GameMode в ней не найдено, значит открывается с глобальным C++ GameMode; не проверялось вручную) |
| `maps/MainMap.umap` | ObjectRedirector на `L_CoopTest` | Нет |
| `Blueprints/BP_Character` | Blueprint от движкового `ACharacter` (не от `ARatRaceCharacter`), использует `IA_Move`/`IA_Jump` | Нет |
| `Blueprints/BP_CoopGameMode` | Blueprint от `AGameModeBase`, использует `BP_Character`, `BP_CoopPlayerController`, `CoopGameInstance` | Нет |
| `Blueprints/BP_CoopPlayerController` | Добавляет `IMC_CoopDefault` | Нет |
| `Blueprints/BP_CoopGameInstance` | Blueprint от `UCoopGameInstance` | Нет (не назначен в конфиге) |
| `Blueprints/BP_GameMode` | ObjectRedirector на `BP_CoopGameMode` | Нет |
| `Input/IA_Move`, `Input/IA_Jump`, `Input/IMC_CoopDefault` | Ассеты ввода прототипа (левый стик / A) | Нет; C++-контроллер создаёт свои аналоги в рантайме |

`Content/test_HLOD0_Instancing.uasset` и `Content/__ExternalActors__/test/` остались от удалённой тестовой карты, к игре не относятся.

Скелетных мешей, сокетов, анимаций и собственных материалов в проекте нет. Весь визуал собран из `/Engine/BasicShapes/*` и `BasicShapeMaterial` (параметр `Color`).

---

## 4. Локальный мультиплеер

### 4.1. Создание игроков

1. Движок создаёт `ULocalPlayer` 0 и его `ARatRacePlayerController`.
2. `ARatRaceGameMode::BeginPlay` вызывает `UGameplayStatics::CreatePlayer(World, ControllerId, true)` для `ControllerId = 1 .. NumLocalPlayers-1`. Каждый вызов создаёт `ULocalPlayer`, `ARatRacePlayerController` и через `RestartPlayer` спавнит `ARatRaceCharacter`.
3. `RestartPlayer` ставит крысу в `SpawnPoints[индекс]` (по углам арены, лицом к центру) и вызывает `SetPlayerColor(PlayerColors[индекс])`: красный, синий, зелёный, жёлтый.
4. Индекс игрока — `ULocalPlayer::GetLocalPlayerIndex()`.

### 4.2. Назначение устройств

- `ARatRaceGameMode::AssignGamepadsToPlayers` сортирует подключённые геймпады по `FInputDeviceId` и через `IPlatformInputDeviceMapper::Internal_ChangeInputDeviceUserMapping` закрепляет N-й геймпад за `PlatformUserId` N-го локального игрока. Клавиатура и мышь остаются у игрока 0.
- При подключении или отключении устройства переназначение выполняется на следующем тике (флаг `bGamepadAssignmentDirty`).
- `FRatRaceGamepadRouter` нужен потому, что Slate доставляет события геймпада по фокусу своего пользователя, а фокус на игровом вьюпорте надёжно есть только у пользователя 0. Роутер перехватывает все события геймпадов до Slate и вызывает `UGameViewportClient::InputKey/InputAxis`, после чего движок сам выбирает `ULocalPlayer` по `PlatformUserId` устройства.
- При `bShowGamepadAssignment = true` (по умолчанию) на экран выводится отладка: назначение устройств и живой ввод.

### 4.3. Ввод каждого игрока

`ARatRacePlayerController` в `SetupInputComponent` создаёт **свои** экземпляры `IA_Move` (Axis2D), `IA_Jump` (Bool), `IA_Grab` (Bool). В `BeginPlay`/`ReceivedPlayer` он добавляет **свой** `IMC_RatRace` в `UEnhancedInputLocalPlayerSubsystem` **своего** `ULocalPlayer`. Общих объектов ввода между игроками нет.

| Действие | Геймпад (каждый игрок) | Клавиатура (только игрок 0, `bKeyboardForFirstPlayer`) |
|---|---|---|
| Движение | Левый стик (radial dead zone `StickDeadZone` = 0.2) | WASD |
| Прыжок | A / Cross (`Gamepad_FaceButton_Bottom`) | Space |
| Захват хвоста (удерживать) | **Правый триггер RT / R2 (`Gamepad_RightTrigger`)** | **E** |

Движение считается относительно yaw общей камеры (стик вверх — от камеры).

### 4.4. Один экран

`UGameViewportClient::SetForceDisableSplitscreen(true)` и `bUseSplitscreen=False`. Рисуется только вид игрока 0, все контроллеры держат view target на `ArenaCamera` (`bAutoManageActiveCameraTarget = false`).

### 4.5. Ограничения и известные проблемы

- **Одиночная игра фактически не поддерживается:** `NumLocalPlayers` ограничен `ClampMin = 2`, а в `BeginPlay` стоит `FMath::Clamp(NumLocalPlayers, 2, 4)`. Всегда создаётся минимум 2 крысы, даже если подключён один геймпад. Это расходится с требованием «1–4 игрока»; в рамках задачи про хвосты не исправлялось.
- Максимум 4 игрока (clamp в GameMode, 4 точки спавна и 4 цвета).
- Игроки создаются сразу при старте, а не при подключении геймпада. Механизма «join/leave» в активном пайплайне нет (`UCoopGameInstance::TryJoinNextPlayer` не используется).
- Клавиатура и первый геймпад управляют одной и той же крысой (игрок 0).
- `Internal_ChangeInputDeviceUserMapping` — «внутренний» API движка, может измениться в следующих версиях UE.

---

## 5. Мини-игры

**Не реализовано.** В коде нет классов, компонентов, ассетов или данных для мини-игр, раундов, очков, результатов, победителя матча или сыра. Переходов между уровнями тоже нет: игра запускается на `L_RatRace` и остаётся там.

Сейчас есть одна «песочница»: `ARatRaceArenaBuilder` строит пол с платформами, `ARatRaceGameMode` возвращает упавших (`Z < RespawnZ`) на точку спавна.

Что из текущей архитектуры пригодится будущей системе мини-игр:
- GameMode, контроллеры и крысы не завязаны на конкретную арену, `ARatRaceArenaBuilder` можно заменить или поставить на уровень вручную (GameMode переиспользует найденный на уровне экземпляр).
- Хвост — самостоятельный компонент с Blueprint-API (`TryGrab`, `Release`, `GetHeldTail`, `GetHolder`, `IsHoldingTail`), мини-игры могут его опрашивать.
- Компонент хвоста корректно освобождает захват в `EndPlay`, поэтому смена уровня или уничтожение крыс не оставляет висящих связей.

Как организовать мини-игры (отдельные GameMode на уровень, подсистема, data assets и т. п.) пока не решено.

---

## 6. Механика хвостов

### 6.1. Классы и компоненты

| Что | Где |
|---|---|
| Визуал хвоста `TailMesh` | `ARatRaceCharacter`, [RatRaceCharacter.cpp](Source/RatRace/RatRaceCharacter.cpp) |
| Точка хвоста и логика `Tail` (`URatRaceTailComponent`) | [RatRaceTailComponent.h](Source/RatRace/RatRaceTailComponent.h), [RatRaceTailComponent.cpp](Source/RatRace/RatRaceTailComponent.cpp) |
| Кнопка захвата и привязка | `ARatRacePlayerController`, [RatRacePlayerController.cpp](Source/RatRace/RatRacePlayerController.cpp) |

Один и тот же `URatRaceTailComponent` выполняет обе роли: «мой хвост, за который можно схватить» и «я держу чужой хвост».

### 6.2. Визуал и крепление

- `TailMesh` — `/Engine/BasicShapes/Cylinder` толщиной 16 uu и длиной ~120 uu, **жёстко** прикреплён к капсуле. Он тянется от нижней части спины (`TailBase` = (−30, 0, −40.5) в локальных координатах капсулы) назад и вниз к кончику (`TailTip` = (−145, 0, −75), чуть выше пола).
- Материал — тот же `UMaterialInstanceDynamic`, что у тела, поэтому хвост всегда цвета своего игрока, в том числе после `SetPlayerColor`.
- Коллизии у хвоста нет: на движение он не влияет.
- Компонент `Tail` (USceneComponent) стоит в `TailTip`. Его мировая позиция — точка, за которую хватают.
- Хвост поворачивается и движется вместе с капсулой. Изгиба и физики у хвоста нет.

### 6.3. Захват

Ввод: **удерживать RT** (игрок 0 может также удерживать **E**).
- `ETriggerEvent::Triggered` срабатывает каждый кадр, пока кнопка зажата. Если крыса ещё ничего не держит, вызывается `TryGrab()`. Поэтому можно зажать RT заранее и подбежать к хвосту.
- `ETriggerEvent::Completed` (кнопку отпустили) вызывает `Release()`.

`TryGrab()`:
1. Точка захвата — `GrabPointOffset` = (60, 0, −70) в локальных координатах актора, то есть перед мордой у пола.
2. Перебираются все `APawn` в мире (их не больше четырёх). Пропускаются: собственный актор (**свой хвост схватить нельзя**), невалидные акторы, акторы без `URatRaceTailComponent`, хвосты, которые уже кто-то держит.
3. Из оставшихся выбирается ближайший кончик хвоста в пределах `GrabReach` = 100 uu.
4. Связь сохраняется с двух сторон через `TWeakObjectPtr`: `HeldTail` у хватающего и `Holder` у хвоста.
5. У схваченной крысы `bOrientRotationToMovement` выключается, исходное значение запоминается.
6. Включается тик компонента хватающего. Без активного захвата компонент не тикает.

**Конфликт захватов.** Хвост может держать только одна крыса. Кто схватил первым, тот и держит, пока не отпустит. Остальные попытки схватить этот хвост возвращают `false`. Если двое нажали RT в одном кадре, побеждает контроллер, ввод которого движок обработал раньше; второй в этом кадре получит отказ. Физических связей нет, поэтому конфликтовать нечему.

Разрешено:
- держать хвост, когда держат твой собственный (цепочки A→B→C);
- взаимный захват A↔B.

### 6.4. Тяга

Физические констрейнты и силы **не используются**: крысы управляются `CharacterMovementComponent`, и физика поверх него дала бы рывки и борьбу с торможением. Тяга сделана как дополнительный **movement input**:

```
Rope    = (центр хватающего − кончик схваченного хвоста), по горизонтали
Stretch = |Rope| − SlackLength
Pull    = clamp(Stretch / PullRampLength, 0, 1) × MaxPullStrength
Схваченный:  AddMovementInput( Rope.normal,  Pull)
Хватающий:   AddMovementInput(−Rope.normal,  Pull × GrabberRecoil)
```

- Добавка складывается со стиком самого игрока, а `CharacterMovement` ограничивает сумму единицей. Поэтому скорость никогда не превышает обычную `MaxWalkSpeed` (600), нет рывков и раскачки. Ходьба, прыжки, ступеньки и коллизии работают как обычно.
- Схваченная крыса каждый кадр поворачивается **спиной (хвостом) к хватающему** с обычной скоростью поворота (`RotationRate.Yaw`). Без этого стик-инпут тяги разворачивал бы её мордой к тянущему, кончик хвоста перескакивал бы на другую сторону, и длина верёвки менялась бы скачком. После освобождения `bOrientRotationToMovement` восстанавливается.
- Параметры по умолчанию (`EditAnywhere`, категория `RatRace|Tail` у компонента `Tail`):

| Параметр | По умолчанию | Смысл |
|---|---|---|
| `GrabPointOffset` | (60, 0, −70) | Точка «рта» хватающего (локально) |
| `GrabReach` | 100 | Радиус захвата от точки рта до кончика хвоста |
| `SlackLength` | 120 | Длина верёвки без тяги |
| `PullRampLength` | 150 | Сверх провиса, на котором тяга доходит до максимума |
| `MaxPullStrength` | 2.0 | Максимальная добавка к инпуту схваченного (1 = полный стик) |
| `GrabberRecoil` | 0.5 | Доля тяги, которую чувствует хватающий |
| `BreakDistance` | 500 | Захват рвётся, если крысы оказались дальше (например, после респавна) |

- Поведение с этими значениями (измерено в автотесте, §7.3): схваченную стоящую крысу тянут со скоростью ~400 uu/s, верёвка держится на ~170 uu. В перетягивании (оба тянут в разные стороны на полном стике) хватающий медленно выигрывает: ~120 uu/s в его сторону.

### 6.5. Освобождение и жизненный цикл

Захват снимается:
- при отпускании RT/E (`Release()`);
- в `URatRaceTailComponent::EndPlay` у любой из сторон: уничтожение крысы, смена или выгрузка уровня, конец PIE;
- в `ARatRacePlayerController::OnUnPossess`: контроллер потерял пауна, отпустить кнопку больше некому;
- в тике, если хватающий или схваченный стали невалидны, связь стала несимметричной или расстояние превысило `BreakDistance`. Последнее покрывает телепорт при падении (`ARatRaceGameMode::Tick`).

Хранение через `TWeakObjectPtr` гарантирует, что код никогда не обращается к уничтоженной крысе.

### 6.6. Ограничения и что учитывать

- Пока хвост держат, схваченная крыса не может развернуться к своему стику: двигаться может, но спиной к хватающему. Так задумано.
- Тяга работает только по горизонтали. В прыжке действует обычный `AirControl` (0.6), поэтому в воздухе тяга слабее.
- Захват проверяет только расстояние, без line of sight: через тонкую стену схватить можно.
- Хвост жёсткий, к хватающему визуально не изгибается. Кроме разворота схваченной крысы других индикаторов захвата нет.
- Если для пауна вызван `SetIgnoreMoveInput`, тяга на него не подействует (она идёт через `AddMovementInput`).
- Не меняйте `bOrientRotationToMovement` у крысы, пока её держат: при освобождении оно будет перезаписано сохранённым значением.

---

## 7. Состояние проекта

### 7.1. До этой задачи (коммит `a3c9f91`)

Локальный мультиплеер на 4 крысы с раздельными геймпадами и роутером, общая камера, рантайм-арена, движение и прыжок, респавн при падении. Blueprint/Input-ассеты прототипа в `Content/coop`. Мини-игр нет.

### 7.2. Изменено в этой задаче

| Файл | Изменение |
|---|---|
| `Source/RatRace/RatRaceTailComponent.h/.cpp` | **Новый** компонент хвоста: захват, тяга, освобождение |
| `Source/RatRace/RatRaceCharacter.h/.cpp` | Добавлены `TailMesh`, компонент `Tail` и геттер `GetTail()`; хвост использует материал тела |
| `Source/RatRace/RatRacePlayerController.h/.cpp` | Добавлены `IA_Grab` (RT; E для игрока 0), обработчики Triggered/Completed и `OnUnPossess` с освобождением захвата |
| `PROJECT_CONTEXT.md` | **Новый**, этот документ |

Blueprint-ассеты, конфиги, GameMode, роутер, арена и настройки физики **не менялись**.

### 7.3. Выполненные проверки

**Проверено фактически:**
- Сборка `RatRaceEditor Win64 Development` через UnrealBuildTool прошла без ошибок и предупреждений.
- Автотест в запущенной игре (`UnrealEditor-Cmd -game -nullrhi`, карта `L_RatRace`, 4 локальных игрока). Драйвер теста был временным файлом и после прогона удалён из проекта. Пройдено 36 из 36 проверок, отдельно при 30 FPS. Проверялось:
  - 4 разных контроллера, LocalPlayer, пауна и компонента хвоста; каждый игрок владеет своей крысой;
  - кончик хвоста поворачивается вместе с крысой;
  - нажатие RT (симулировано через `APlayerController::InputKey` конкретного игрока) хватает хвост только у этого игрока; остальные ничего не хватают;
  - второй хватающий получает отказ, первый сохраняет хвост;
  - стоящую крысу утаскивает (~440 uu за 1.5 с, установившаяся скорость 400/400, верёвка 170, без раскачки);
  - в перетягивании хватающий выигрывает; скорость не превышает скорость ходьбы;
  - отпускание RT освобождает хвост, отключает тик и восстанавливает `bOrientRotationToMovement`;
  - после освобождения обе крысы свободно ходят; прыжок по A у игрока 1 работает и не задевает игрока 0;
  - одновременные захваты игроками 0 и 3 попадают в правильные хвосты;
  - захват рвётся при телепорте на большое расстояние;
  - уничтожение схваченной крысы или хватающего не оставляет активного захвата.

**Не проверено:**
- Реальные геймпады. В тесте ввод подавался прямо в PlayerController, поэтому путь Slate → `FRatRaceGamepadRouter` → viewport для правого триггера не проверялся. Предполагается, что он работает так же, как для A: роутер передаёт любые `IsGamepadKey()`-клавиши, включая `Gamepad_RightTrigger`.
- Внешний вид хвоста: тест шёл без рендера, размеры и читаемость с камеры арены не оценивались.
- Игровой баланс тяги (насколько это весело) — нужен плейтест.
- Режим с одним игроком (см. §4.5).

### 7.4. Известные проблемы, найденные по ходу работы (не исправлялись)

- Минимум 2 игрока в `ARatRaceGameMode` (§4.5).
- Нет системы мини-игр, очков, победителя и награды (§5).
- Ассеты прототипа (`BP_*`, `IA_*`, `IMC_CoopDefault`) и `UCoopGameInstance` не используются активным пайплайном.
- Отладочный вывод назначения геймпадов включён по умолчанию (`bShowGamepadAssignment`).

### 7.5. Ручные действия

- Проверить захват хвоста на реальных геймпадах (2 и 4 игрока) в PIE или Standalone.
- Оценить видимость хвоста с камеры и при необходимости подстроить `TailThickness`/`TailTip` в [RatRaceCharacter.cpp](Source/RatRace/RatRaceCharacter.cpp), а параметры тяги — в Details компонента `Tail`.
- Настраивать Blueprint-ассеты для хвостов **не требуется**: всё создаётся в C++.

---

## 8. Правила дальнейшей работы с проектом

1. Поддерживать от одного до четырёх локальных игроков.
2. Использовать один общий экран без сплитскрина.
3. Сохранять управление отдельными контроллерами.
4. Сочетать C++ и Blueprints в соответствии с текущей архитектурой (сейчас основа на C++, Blueprint — для настройки и надстроек).
5. Сохранять модульность мини-игр.
6. Переиспользовать существующие классы и системы.
7. Не переписывать работающий код без необходимости.
8. Не добавлять игровые механики и функции, которые не были запрошены.
9. При изменениях, существенно меняющих архитектуру или поведение проекта, обновлять `PROJECT_CONTEXT.md`.
