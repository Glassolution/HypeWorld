"""Envia código Python para o editor da Unreal que está aberto e mostra a resposta.

Usa o remote_execution.py oficial da engine. No editor, precisa estar ligado:
Project Settings > Plugins > Python > "Enable Remote Execution?".

Uso:
    python3 Tools/ue_exec.py "print(unreal.SystemLibrary.get_engine_version())"
    python3 Tools/ue_exec.py -f caminho/do/script.py
"""
import sys
import time

sys.path.append("/Users/Shared/Epic Games/UE_5.8/Engine/Plugins/Experimental/PythonScriptPlugin/Content/Python")
import remote_execution as rex  # noqa: E402


class _MacLoopbackBroadcast(rex._RemoteExecutionBroadcastConnection):
    """Correção para macOS: um socket preso a 127.0.0.1 não recebe multicast no Mac.

    - Nosso socket escuta em 0.0.0.0 (mas só entra no grupo pela interface local 127.0.0.1),
      então recebe as respostas do editor.
    - Mandamos ping/open_connection direto para 127.0.0.1:6766 (unicast), que é onde o editor escuta.
    O editor continua preso a 127.0.0.1, ou seja, invisível para a rede Wi-Fi.
    """

    def _init_broadcast_socket(self):
        s = rex._socket
        local = s.inet_aton("127.0.0.1")
        self._broadcast_socket = s.socket(s.AF_INET, s.SOCK_DGRAM, s.IPPROTO_UDP)
        self._broadcast_socket.setsockopt(s.SOL_SOCKET, s.SO_REUSEPORT, 1)
        self._broadcast_socket.bind(("0.0.0.0", self._config.multicast_group_endpoint[1]))
        self._broadcast_socket.setsockopt(s.IPPROTO_IP, s.IP_MULTICAST_LOOP, 1)
        self._broadcast_socket.setsockopt(s.IPPROTO_IP, s.IP_MULTICAST_TTL, 0)
        self._broadcast_socket.setsockopt(s.IPPROTO_IP, s.IP_MULTICAST_IF, local)
        self._broadcast_socket.setsockopt(
            s.IPPROTO_IP, s.IP_ADD_MEMBERSHIP, s.inet_aton(self._config.multicast_group_endpoint[0]) + local)
        self._broadcast_socket.settimeout(0.1)

    def _broadcast_message(self, message):
        self._broadcast_socket.sendto(message.to_json_bytes(), ("127.0.0.1", self._config.multicast_group_endpoint[1]))


rex._RemoteExecutionBroadcastConnection = _MacLoopbackBroadcast


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    if sys.argv[1] == "-f":
        with open(sys.argv[2], encoding="utf-8") as f:
            command = f.read()
    else:
        command = sys.argv[1]

    remote = rex.RemoteExecution()
    remote.start()
    try:
        # Espera o editor responder ao "ping" na rede local (até 15 s; o editor pode estar ocupado).
        for _ in range(150):
            if remote.remote_nodes:
                break
            time.sleep(0.1)
        if not remote.remote_nodes:
            print("ERRO: nenhum editor encontrado. O editor está aberto com 'Enable Remote Execution' ligado?")
            sys.exit(2)

        remote.open_command_connection(remote.remote_nodes[0]["node_id"])
        result = remote.run_command(command, unattended=True)
        for line in result.get("output", []):
            print(line.get("output", "").rstrip())
        if not result.get("success"):
            print("FALHOU:", result.get("result"))
            sys.exit(3)
    finally:
        remote.stop()


if __name__ == "__main__":
    main()
