from typing import Final

# Blocco 1
N_SERV_1: Final = 5.0
LAMBDA_1: Final = 5.0
MU_1: Final = 5.0
# Blocco 2
P_FEVER: Final = 0.1
P_ONLINE: Final = 0.6
P_DROP_OFF: Final = 0.4
N_SERV_2: Final = 20.0
N_SERV_DROP_OFF: Final = 3.0
MU_2: Final = 0.2
MU_DROP_OFF: Final = 1.0
# Blocco 3
N_SERV_3: Final = 20.0
SERVICE_3: Final = 3.0


def block_one():
    lambda_i = LAMBDA_1 / N_SERV_1
    ro_i = lambda_i / MU_1
    service = 1 / MU_1
    return service / (1 - ro_i)


def block_two():
    lambda_tot = LAMBDA_1 * (1 - P_FEVER)
    lambda_i = (1 - P_ONLINE) * lambda_tot / N_SERV_2
    ro_i = lambda_i / MU_2
    service = 1 / MU_2
    response = service / (1 - ro_i)
    lambda_online = P_ONLINE * lambda_tot
    lambda_drop_off_i = lambda_online * P_DROP_OFF / N_SERV_DROP_OFF
    ro_drop_off_i = lambda_drop_off_i / MU_DROP_OFF
    service_drop_off = 1 / MU_DROP_OFF
    response_drop_off = service_drop_off / (1 - ro_drop_off_i)
    return (1 - P_ONLINE) * response + P_ONLINE * P_DROP_OFF * response_drop_off


def block_three():
    lambda_tot = LAMBDA_1 * (1 - P_FEVER)
    lambda_i = lambda_tot / N_SERV_3
    mu = 1 / SERVICE_3
    ro_i = lambda_i / mu
    return SERVICE_3 / (1 - ro_i)


if __name__ == '__main__':
    one = block_one()
    two = block_two()
    three = block_three()
    print(f"Blocco 1: {one}\nBlocco 2: {two}\nBlocco 3: {three}\nTot: {one + two + three}")
