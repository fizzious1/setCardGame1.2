import { HouseLayout, Room, RoomConnection } from '../../shared/types';

export function createHouseLayout(): HouseLayout {
  const rooms: Room[] = [
    {
      id: 'living_room',
      name: 'Living Room',
      type: 'living_room',
      x: 300,
      y: 250,
      width: 200,
      height: 180,
      capacity: 8,
    },
    {
      id: 'kitchen',
      name: 'Kitchen',
      type: 'kitchen',
      x: 520,
      y: 250,
      width: 150,
      height: 150,
      capacity: 6,
    },
    {
      id: 'bedroom1',
      name: 'Bedroom 1',
      type: 'bedroom',
      x: 100,
      y: 100,
      width: 150,
      height: 130,
      capacity: 4,
    },
    {
      id: 'bedroom2',
      name: 'Bedroom 2',
      type: 'bedroom',
      x: 100,
      y: 400,
      width: 150,
      height: 130,
      capacity: 4,
    },
    {
      id: 'garden',
      name: 'Garden',
      type: 'garden',
      x: 520,
      y: 80,
      width: 200,
      height: 150,
      capacity: 8,
    },
    {
      id: 'confessional',
      name: 'Confessional Room',
      type: 'confessional',
      x: 100,
      y: 260,
      width: 100,
      height: 100,
      capacity: 1,
    },
    {
      id: 'nomination_room',
      name: 'Nomination Room',
      type: 'nomination_room',
      x: 520,
      y: 420,
      width: 150,
      height: 120,
      capacity: 8,
    },
    {
      id: 'competition_arena',
      name: 'Competition Arena',
      type: 'competition_arena',
      x: 300,
      y: 50,
      width: 200,
      height: 150,
      capacity: 8,
    },
    {
      id: 'hallway',
      name: 'Hallway',
      type: 'hallway',
      x: 270,
      y: 180,
      width: 230,
      height: 50,
      capacity: 8,
    },
  ];

  const connections: RoomConnection[] = [
    // Hallway connects to everything
    { from: 'hallway', to: 'living_room' },
    { from: 'hallway', to: 'kitchen' },
    { from: 'hallway', to: 'bedroom1' },
    { from: 'hallway', to: 'bedroom2' },
    { from: 'hallway', to: 'garden' },
    { from: 'hallway', to: 'confessional' },
    { from: 'hallway', to: 'nomination_room' },
    { from: 'hallway', to: 'competition_arena' },
    // Direct connections
    { from: 'living_room', to: 'kitchen' },
    { from: 'living_room', to: 'garden' },
    { from: 'kitchen', to: 'garden' },
    // Bidirectional: add reverse
    { from: 'living_room', to: 'hallway' },
    { from: 'kitchen', to: 'hallway' },
    { from: 'bedroom1', to: 'hallway' },
    { from: 'bedroom2', to: 'hallway' },
    { from: 'garden', to: 'hallway' },
    { from: 'confessional', to: 'hallway' },
    { from: 'nomination_room', to: 'hallway' },
    { from: 'competition_arena', to: 'hallway' },
    { from: 'kitchen', to: 'living_room' },
    { from: 'garden', to: 'living_room' },
    { from: 'garden', to: 'kitchen' },
  ];

  return { rooms, connections };
}

export function getConnectedRooms(house: HouseLayout, roomId: string): string[] {
  return house.connections
    .filter((c) => c.from === roomId)
    .map((c) => c.to);
}
