import {Dispatch, SetStateAction, useContext, useEffect, useState} from "react";
import {AppContext} from "../Contexts/AppContext";
import Http from "../Services/Http";
import {baseUrl} from "../Services/WebApi";

type ConfigField = {
    name: string;
    type: "int" | "float" | "bool";
    label: string;
}

const configFields: ConfigField[] = [
    { name: "x1AdcMin", type: "int", label: "x1AdcMin" },
    { name: "x1AdcMid", type: "int", label: "x1AdcMid" },
    { name: "x1AdcMax", type: "int", label: "x1AdcMax" },

    { name: "y1AdcMin", type: "int", label: "y1AdcMin" },
    { name: "y1AdcMid", type: "int", label: "y1AdcMid" },
    { name: "y1AdcMax", type: "int", label: "y1AdcMax" },

    { name: "z1AdcMin", type: "int", label: "z1AdcMin" },
    { name: "z1AdcMax", type: "int", label: "z1AdcMax" },

    { name: "x2AdcMin", type: "int", label: "x2AdcMin" },
    { name: "x2AdcMid", type: "int", label: "x2AdcMid" },
    { name: "x2AdcMax", type: "int", label: "x2AdcMax" },

    { name: "y2AdcMin", type: "int", label: "y2AdcMin" },
    { name: "y2AdcMid", type: "int", label: "y2AdcMid" },
    { name: "y2AdcMax", type: "int", label: "y2AdcMax" },

    { name: "z2AdcMin", type: "int", label: "z2AdcMin" },
    { name: "z2AdcMax", type: "int", label: "z2AdcMax" },

    // { name: "analogLeftTrigger", type: "bool", label: "analogLeftTrigger" },
    { name: "digitalLeftTriggerThresholdPercent", type: "float", label: "digitalLeftTriggerThresholdPercent" },
    // { name: "analogRightTrigger", type: "bool", label: "analogRightTrigger" },
    { name: "digitalRightTriggerThresholdPercent", type: "float", label: "digitalRightTriggerThresholdPercent" },
];


export default function F310ModConfig() {
    const { setLoading } = useContext(AppContext);

    const [data, setData] = useState<Record<string, any> | null>(null);

    async function getConfig() {
        return Http.get(`${baseUrl}/api/getF310Config`)
            .then((response) => response.data)
            .catch(console.error)
    }

    async function setConfig(payload: any) {
        return Http.post(`${baseUrl}/api/setF310Config`, payload)
            .then((response) => response.data)
            .catch(console.error)
    }

    useEffect(() => {
        async function fetchData() {
            setLoading(true);
            const data = await getConfig();
            setLoading(false);
            setData(data);
        }

        fetchData();
    }, []);

    return (
        <div>
            {"F310 Mod Config"}
            <table>
                <tbody>
                    {configFields.map(field => (
                        <tr key={field.name}>
                            <td>{field.label}</td>
                            <td>
                                {field.type === "bool" ? (
                                    <input
                                        type="checkbox"
                                        checked={!!data?.[field.name]}
                                        onChange={(e) => {
                                            setData(prev => ({
                                                ...(prev ?? {}),
                                                [field.name]: e.target.checked,
                                            }));
                                        }}
                                    />
                                ) : (
                                    <input
                                        type="number"
                                        step={field.type === "float" ? "any" : 1}
                                        value={data?.[field.name] ?? ""}
                                        onChange={(e) => {
                                            const raw = e.target.value;
                                            const parsed = field.type === "float"
                                                ? parseFloat(raw)
                                                : parseInt(raw, 10);
                                            const value = isNaN(parsed) ? 0 : parsed;
                                            setData(prev => ({
                                                ...(prev ?? {}),
                                                [field.name]: value,
                                            }));
                                        }}
                                    />
                                )}
                            </td>
                        </tr>
                    ))}
                </tbody>
            </table>
            <div style={{ marginTop: 12 }}>
                <button
                    onClick={async () => {
                        if (!data) return;
                        setLoading(true);
                        try {
                            await setConfig(data);
                        } finally {
                            setLoading(false);
                        }
                    }}
                >
                    Save
                </button>
            </div>
        </div>
    )
}